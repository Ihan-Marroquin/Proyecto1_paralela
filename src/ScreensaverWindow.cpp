#include "ScreensaverWindow.h"

#include "AuroraRenderer.h"
#include "AuroraSimulation.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#ifdef _WIN32

#include <windows.h>

namespace {

struct WindowState {
    bool running = true;
    bool paused = false;
    bool saveRequested = false;
    bool fullscreenToggleRequested = false;
    bool fullscreen = false;
    RenderMode mode = RenderMode::Optimized;
    DWORD windowedStyle = WS_OVERLAPPEDWINDOW;
    WINDOWPLACEMENT windowedPlacement{};
};

LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wordParameter,
                                 LPARAM longParameter) {
    WindowState* state = reinterpret_cast<WindowState*>(
        GetWindowLongPtr(window, GWLP_USERDATA));

    if (message == WM_NCCREATE) {
        const auto* creation = reinterpret_cast<const CREATESTRUCT*>(longParameter);
        state = static_cast<WindowState*>(creation->lpCreateParams);
        SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }

    switch (message) {
        case WM_ERASEBKGND:
            return 1;
        case WM_KEYDOWN:
            if (state == nullptr) {
                break;
            }
            if (wordParameter == VK_ESCAPE) {
                DestroyWindow(window);
            } else if (wordParameter == VK_SPACE) {
                state->paused = !state->paused;
            } else if (wordParameter == 'S') {
                state->saveRequested = true;
            } else if (wordParameter == 'F') {
                state->fullscreenToggleRequested = true;
            } else if (wordParameter == '1') {
                state->mode = RenderMode::Sequential;
            } else if (wordParameter == '2') {
                state->mode = RenderMode::Parallel;
            } else if (wordParameter == '3') {
                state->mode = RenderMode::Optimized;
            }
            return 0;
        case WM_CLOSE:
            DestroyWindow(window);
            return 0;
        case WM_DESTROY:
            if (state != nullptr) {
                state->running = false;
            }
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProc(window, message, wordParameter, longParameter);
}

bool drawFramebuffer(HWND window, const AuroraRenderer& renderer) {
    RECT clientArea{};
    if (GetClientRect(window, &clientArea) == 0) {
        return false;
    }

    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = renderer.width();
    bitmapInfo.bmiHeader.biHeight = -renderer.height(); // negativo: origen en la esquina superior
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    HDC deviceContext = GetDC(window);
    if (deviceContext == nullptr) {
        return false;
    }

    const int clientWidth = clientArea.right - clientArea.left;
    const int clientHeight = clientArea.bottom - clientArea.top;
    SetStretchBltMode(deviceContext, COLORONCOLOR);
    const int copiedLines = StretchDIBits(
        deviceContext, 0, 0, clientWidth, clientHeight, 0, 0, renderer.width(), renderer.height(),
        renderer.pixels().data(), &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(window, deviceContext);
    return copiedLines != 0 && copiedLines != static_cast<int>(GDI_ERROR);
}

void toggleFullscreen(HWND window, WindowState& state) {
    if (!state.fullscreen) {
        state.windowedStyle = static_cast<DWORD>(GetWindowLongPtr(window, GWL_STYLE));
        state.windowedPlacement.length = sizeof(WINDOWPLACEMENT);

        MONITORINFO monitorInfo{};
        monitorInfo.cbSize = sizeof(MONITORINFO);
        if (GetWindowPlacement(window, &state.windowedPlacement) != 0 &&
            GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitorInfo) != 0) {
            SetWindowLongPtr(window, GWL_STYLE,
                             static_cast<LONG_PTR>(state.windowedStyle & ~WS_OVERLAPPEDWINDOW));
            SetWindowPos(window, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                         monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                         monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            state.fullscreen = true;
        }
    } else {
        SetWindowLongPtr(window, GWL_STYLE, static_cast<LONG_PTR>(state.windowedStyle));
        SetWindowPlacement(window, &state.windowedPlacement);
        SetWindowPos(window, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER |
                         SWP_FRAMECHANGED);
        state.fullscreen = false;
    }
}

} // namespace

int runScreensaverWindow(const AppConfig& config) {
    const HINSTANCE application = GetModuleHandle(nullptr);
    const char* className = "AuroraParalelaWindow";

    WNDCLASS windowClass{};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = windowProcedure;
    windowClass.hInstance = application;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = className;
    if (RegisterClass(&windowClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        std::cerr << "No se pudo registrar la ventana. Codigo: " << GetLastError() << '\n';
        return 1;
    }

    RECT windowArea{0, 0, config.width, config.height};
    const DWORD windowStyle = WS_OVERLAPPEDWINDOW;
    if (AdjustWindowRect(&windowArea, windowStyle, FALSE) == 0) {
        std::cerr << "No se pudo calcular el tamano de la ventana.\n";
        return 1;
    }

    WindowState state;
    state.mode = config.mode;
    HWND window = CreateWindowEx(
        0, className, "Aurora Paralela", windowStyle, CW_USEDEFAULT, CW_USEDEFAULT,
        windowArea.right - windowArea.left, windowArea.bottom - windowArea.top, nullptr, nullptr,
        application, &state);
    if (window == nullptr) {
        std::cerr << "No se pudo crear la ventana. Codigo: " << GetLastError() << '\n';
        return 1;
    }

    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);

    AuroraSimulation simulation(config.width, config.height, config.sourceCount, config.seed);
    AuroraRenderer renderer(config.width, config.height);
    auto previousFrame = std::chrono::steady_clock::now();
    auto titleUpdate = previousFrame;
    int framesSinceTitleUpdate = 0;
    double lastRenderMilliseconds = 0.0;

    while (state.running) {
        MSG message{};
        while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE) != 0) {
            if (message.message == WM_QUIT) {
                state.running = false;
                break;
            }
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        if (!state.running) {
            break;
        }
        if (state.fullscreenToggleRequested) {
            toggleFullscreen(window, state);
            state.fullscreenToggleRequested = false;
        }
        if (state.paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            previousFrame = std::chrono::steady_clock::now();
            continue;
        }

        const auto frameStart = std::chrono::steady_clock::now();
        const float deltaSeconds = std::chrono::duration<float>(frameStart - previousFrame).count();
        previousFrame = frameStart;
        simulation.update(deltaSeconds);

        const auto renderStart = std::chrono::steady_clock::now();
        renderer.render(simulation, state.mode);
        const auto renderEnd = std::chrono::steady_clock::now();
        lastRenderMilliseconds =
            std::chrono::duration<double, std::milli>(renderEnd - renderStart).count();

        if (!drawFramebuffer(window, renderer)) {
            std::cerr << "No se pudo copiar el framebuffer a la ventana.\n";
            DestroyWindow(window);
            return 1;
        }

        if (state.saveRequested) {
            const std::string path = config.outputPath.empty() ? "results/captura.ppm" : config.outputPath;
            std::string error;
            if (renderer.savePpm(path, error)) {
                std::cout << "Captura guardada en: " << path << '\n';
            } else {
                std::cerr << "Error: " << error << '\n';
            }
            state.saveRequested = false;
        }

        ++framesSinceTitleUpdate;
        const double titleSeconds = std::chrono::duration<double>(renderEnd - titleUpdate).count();
        if (titleSeconds >= 0.5) {
            const double framesPerSecond = static_cast<double>(framesSinceTitleUpdate) / titleSeconds;
            std::ostringstream title;
            title.precision(1);
            title << std::fixed << "Aurora Paralela | " << renderModeName(state.mode)
                  << " | FPS " << framesPerSecond
                  << " | render " << lastRenderMilliseconds << " ms | N=" << config.sourceCount;
            SetWindowText(window, title.str().c_str());
            framesSinceTitleUpdate = 0;
            titleUpdate = renderEnd;
        }

        if (config.targetFps > 0) {
            const auto targetFrameTime =
                std::chrono::duration<double>(1.0 / static_cast<double>(config.targetFps));
            const auto targetEnd = frameStart +
                                   std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                       targetFrameTime);
            std::this_thread::sleep_until(targetEnd);
        }
    }
    return 0;
}

#else

int runScreensaverWindow(const AppConfig&) {
    std::cerr << "La ventana interactiva de esta version requiere Windows.\n";
    return 1;
}

#endif

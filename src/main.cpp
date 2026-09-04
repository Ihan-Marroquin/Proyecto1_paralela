#include "AppConfig.h"
#include "AuroraRenderer.h"
#include "AuroraSimulation.h"
#include "Benchmark.h"
#include "ScreensaverWindow.h"

#include <chrono>
#include <iostream>
#include <omp.h>

namespace {

int runHeadless(const AppConfig& config) {
    AuroraSimulation simulation(config.width, config.height, config.sourceCount, config.seed);
    AuroraRenderer renderer(config.width, config.height);

    const int simulationFps = config.targetFps > 0 ? config.targetFps : 60;
    const int frameCount = config.durationSeconds * simulationFps;
    const float deltaSeconds = 1.0F / static_cast<float>(simulationFps);

    const auto start = std::chrono::steady_clock::now();
    for (int frame = 0; frame < frameCount; ++frame) {
        simulation.update(deltaSeconds);
        renderer.render(simulation, config.mode);
    }
    const auto end = std::chrono::steady_clock::now();
    const double elapsedMilliseconds =
        std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "Modo: " << renderModeName(config.mode) << '\n'
              << "Canvas: " << config.width << 'x' << config.height << '\n'
              << "Fuentes: " << config.sourceCount << '\n'
              << "Cuadros: " << frameCount << '\n'
              << "Tiempo total: " << elapsedMilliseconds << " ms\n"
              << "Promedio por cuadro: " << elapsedMilliseconds / frameCount << " ms\n"
              << "Checksum: " << renderer.checksum() << '\n';

    if (!config.outputPath.empty()) {
        std::string error;
        if (!renderer.savePpm(config.outputPath, error)) {
            std::cerr << "Error: " << error << '\n';
            return 1;
        }
        std::cout << "Imagen guardada en: " << config.outputPath << '\n';
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    AppConfig config;
    std::string error;
    if (!parseArguments(argc, argv, config, error)) {
        std::cerr << "Error: " << error << "\n\n";
        printUsage(std::cerr, argv[0]);
        return 2;
    }
    if (config.showHelp) {
        printUsage(std::cout, argv[0]);
        return 0;
    }

    if (config.threadCount == 0) {
        config.threadCount = omp_get_max_threads();
    }
    omp_set_dynamic(0);
    omp_set_num_threads(config.threadCount);

    if (config.selfTest) {
        std::cerr << "Las pruebas internas aun no estan disponibles.\n";
        return 1;
    }
    if (config.benchmark) {
        return runBenchmark(config);
    }
    if (config.mode == RenderMode::Optimized) {
        std::cerr << "El modo optimized aun no esta disponible.\n";
        return 1;
    }
    return config.headless ? runHeadless(config) : runScreensaverWindow(config);
}

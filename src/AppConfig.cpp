#include "AppConfig.h"

#include <charconv>
#include <limits>
#include <ostream>
#include <string_view>

namespace {

bool parseInteger(std::string_view text, int minimum, int maximum, int& value) {
    if (text.empty()) {
        return false;
    }

    int parsed = 0;
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end || parsed < minimum || parsed > maximum) {
        return false;
    }

    value = parsed;
    return true;
}

bool parseSeed(std::string_view text, std::uint32_t& value) {
    unsigned long long parsed = 0;
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end ||
        parsed > std::numeric_limits<std::uint32_t>::max()) {
        return false;
    }

    value = static_cast<std::uint32_t>(parsed);
    return true;
}

bool requireValue(int argc, char* argv[], int& index, std::string_view option,
                  std::string_view& value, std::string& error) {
    if (index + 1 >= argc) {
        error = "Falta el valor de " + std::string(option) + ".";
        return false;
    }
    value = argv[++index];
    return true;
}

} // namespace

const char* renderModeName(RenderMode mode) {
    switch (mode) {
        case RenderMode::Sequential:
            return "sequential";
        case RenderMode::Parallel:
            return "parallel";
        case RenderMode::Optimized:
            return "optimized";
    }
    return "unknown";
}

bool parseArguments(int argc, char* argv[], AppConfig& config, std::string& error) {
    bool hasSourceCount = false;

    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];

        if (argument == "--help" || argument == "-h") {
            config.showHelp = true;
            continue;
        }
        if (argument == "--benchmark") {
            config.benchmark = true;
            config.headless = true;
            continue;
        }
        if (argument == "--headless") {
            config.headless = true;
            continue;
        }
        if (argument == "--self-test") {
            config.selfTest = true;
            config.headless = true;
            continue;
        }

        if (!argument.empty() && argument.front() != '-') {
            if (hasSourceCount || !parseInteger(argument, 1, 512, config.sourceCount)) {
                error = "N debe ser un entero entre 1 y 512 y solo puede indicarse una vez.";
                return false;
            }
            hasSourceCount = true;
            continue;
        }

        std::string_view value;
        if (argument == "--mode") {
            if (!requireValue(argc, argv, index, argument, value, error)) {
                return false;
            }
            if (value == "sequential") {
                config.mode = RenderMode::Sequential;
            } else if (value == "parallel") {
                config.mode = RenderMode::Parallel;
            } else if (value == "optimized") {
                config.mode = RenderMode::Optimized;
            } else {
                error = "El modo debe ser sequential, parallel u optimized.";
                return false;
            }
        } else if (argument == "--width") {
            if (!requireValue(argc, argv, index, argument, value, error) ||
                !parseInteger(value, 640, 3840, config.width)) {
                error = "El ancho debe ser un entero entre 640 y 3840.";
                return false;
            }
        } else if (argument == "--height") {
            if (!requireValue(argc, argv, index, argument, value, error) ||
                !parseInteger(value, 480, 2160, config.height)) {
                error = "El alto debe ser un entero entre 480 y 2160.";
                return false;
            }
        } else if (argument == "--threads") {
            if (!requireValue(argc, argv, index, argument, value, error) ||
                !parseInteger(value, 1, 256, config.threadCount)) {
                error = "La cantidad de hilos debe ser un entero entre 1 y 256.";
                return false;
            }
        } else if (argument == "--fps") {
            if (!requireValue(argc, argv, index, argument, value, error) ||
                !parseInteger(value, 0, 240, config.targetFps)) {
                error = "Los FPS deben estar entre 0 (sin limite) y 240.";
                return false;
            }
        } else if (argument == "--seconds") {
            if (!requireValue(argc, argv, index, argument, value, error) ||
                !parseInteger(value, 1, 3600, config.durationSeconds)) {
                error = "La duracion debe estar entre 1 y 3600 segundos.";
                return false;
            }
        } else if (argument == "--trials") {
            if (!requireValue(argc, argv, index, argument, value, error) ||
                !parseInteger(value, 10, 1000, config.trials)) {
                error = "Las pruebas deben incluir entre 10 y 1000 mediciones.";
                return false;
            }
        } else if (argument == "--seed") {
            if (!requireValue(argc, argv, index, argument, value, error) ||
                !parseSeed(value, config.seed)) {
                error = "La semilla debe ser un entero sin signo de 32 bits.";
                return false;
            }
        } else if (argument == "--output") {
            if (!requireValue(argc, argv, index, argument, value, error) || value.empty()) {
                error = "La ruta de salida no puede estar vacia.";
                return false;
            }
            config.outputPath = std::string(value);
        } else if (argument == "--csv") {
            if (!requireValue(argc, argv, index, argument, value, error) || value.empty()) {
                error = "La ruta del CSV no puede estar vacia.";
                return false;
            }
            config.benchmarkPath = std::string(value);
        } else {
            error = "Opcion desconocida: " + std::string(argument);
            return false;
        }
    }

    if (!config.showHelp && !hasSourceCount) {
        error = "Falta N, la cantidad de fuentes luminosas.";
        return false;
    }
    return true;
}

void printUsage(std::ostream& output, const char* programName) {
    output
        << "Uso: " << programName << " N [opciones]\n\n"
        << "N es la cantidad de fuentes luminosas (1 a 512).\n\n"
        << "Opciones:\n"
        << "  --mode MODO       sequential, parallel u optimized\n"
        << "  --width PIXELES   ancho del canvas (minimo 640)\n"
        << "  --height PIXELES  alto del canvas (minimo 480)\n"
        << "  --threads N       hilos de OpenMP\n"
        << "  --fps N           limite de FPS; 0 lo desactiva\n"
        << "  --seconds N       duracion del modo sin ventana\n"
        << "  --seed N          semilla para una ejecucion reproducible\n"
        << "  --headless        ejecutar sin abrir una ventana\n"
        << "  --output RUTA     guardar el ultimo cuadro como imagen PPM\n"
        << "  --benchmark       comparar los tres modos sin ventana\n"
        << "  --trials N        mediciones del benchmark (minimo 10)\n"
        << "  --csv RUTA        archivo CSV para las mediciones\n"
        << "  --self-test       comprobar consistencia de los renderizadores\n"
        << "  --help, -h        mostrar esta ayuda\n";
}

#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

enum class RenderMode {
    Sequential,
    Parallel,
    Optimized
};

struct AppConfig {
    int sourceCount = 0;
    int width = 960;
    int height = 540;
    int threadCount = 0;
    int targetFps = 60;
    int durationSeconds = 10;
    int trials = 10;
    std::uint32_t seed = 2026;
    RenderMode mode = RenderMode::Optimized;
    bool benchmark = false;
    bool headless = false;
    bool selfTest = false;
    bool showHelp = false;
    std::string outputPath;
};

// Devuelve false y deja un mensaje legible cuando algun argumento no es valido.
bool parseArguments(int argc, char* argv[], AppConfig& config, std::string& error);
void printUsage(std::ostream& output, const char* programName);
const char* renderModeName(RenderMode mode);


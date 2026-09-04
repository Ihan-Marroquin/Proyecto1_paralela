#include "Benchmark.h"

#include "AuroraRenderer.h"
#include "AuroraSimulation.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace {

struct ModeMeasurements {
    RenderMode mode;
    std::vector<double> milliseconds;
};

struct ModeSummary {
    double average = 0.0;
    double minimum = 0.0;
    double maximum = 0.0;
    double standardDeviation = 0.0;
};

ModeSummary summarize(const std::vector<double>& values) {
    ModeSummary summary;
    summary.average = std::accumulate(values.begin(), values.end(), 0.0) /
                      static_cast<double>(values.size());
    const auto limits = std::minmax_element(values.begin(), values.end());
    summary.minimum = *limits.first;
    summary.maximum = *limits.second;

    double squaredDifferences = 0.0;
    for (const double value : values) {
        const double difference = value - summary.average;
        squaredDifferences += difference * difference;
    }
    summary.standardDeviation = std::sqrt(squaredDifferences / static_cast<double>(values.size()));
    return summary;
}

bool prepareCsv(const std::filesystem::path& path, std::ofstream& output, std::string& error) {
    std::error_code directoryError;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), directoryError);
        if (directoryError) {
            error = "No se pudo crear el directorio del CSV: " + directoryError.message();
            return false;
        }
    }

    output.open(path);
    if (!output) {
        error = "No se pudo abrir el CSV: " + path.string();
        return false;
    }
    return true;
}

} // namespace

int runBenchmark(const AppConfig& config) {
    AuroraSimulation simulation(config.width, config.height, config.sourceCount, config.seed);
    AuroraRenderer renderer(config.width, config.height);
    std::vector<ModeMeasurements> measurements = {
        {RenderMode::Sequential, {}},
        {RenderMode::Parallel, {}},
    };
    for (auto& mode : measurements) {
        mode.milliseconds.reserve(static_cast<std::size_t>(config.trials));
    }

    // Calentamiento: carga paginas de memoria y crea el equipo de hilos antes de medir.
    simulation.update(1.0F / 60.0F);
    for (const auto& mode : measurements) {
        renderer.render(simulation, mode.mode);
    }

    std::ofstream csv;
    std::string error;
    if (!prepareCsv(config.benchmarkPath, csv, error)) {
        std::cerr << "Error: " << error << '\n';
        return 1;
    }
    csv << "trial,sources,width,height,threads,mode,milliseconds,checksum\n";
    csv << std::fixed << std::setprecision(6);

    std::cout << "Benchmark: " << config.trials << " mediciones por modo, "
              << config.threadCount << " hilos\n";

    for (int trial = 0; trial < config.trials; ++trial) {
        simulation.update(1.0F / 60.0F);
        std::vector<std::uint64_t> checksums(measurements.size());

        // Rotar el orden reduce la ventaja de cache de un modo sobre otro.
        for (std::size_t offset = 0; offset < measurements.size(); ++offset) {
            const std::size_t modeIndex =
                (static_cast<std::size_t>(trial) + offset) % measurements.size();
            ModeMeasurements& mode = measurements[modeIndex];

            const auto start = std::chrono::steady_clock::now();
            renderer.render(simulation, mode.mode);
            const auto end = std::chrono::steady_clock::now();
            const double elapsed =
                std::chrono::duration<double, std::milli>(end - start).count();
            mode.milliseconds.push_back(elapsed);
            checksums[modeIndex] = renderer.checksum();

            csv << (trial + 1) << ',' << config.sourceCount << ',' << config.width << ','
                << config.height << ',' << config.threadCount << ',' << renderModeName(mode.mode)
                << ',' << elapsed << ',' << checksums[modeIndex] << '\n';
        }

        if (!std::all_of(checksums.begin() + 1, checksums.end(),
                         [&](std::uint64_t value) { return value == checksums.front(); })) {
            std::cerr << "Error: los modos generaron resultados distintos en la medicion "
                      << (trial + 1) << ".\n";
            return 1;
        }
    }

    const ModeSummary sequentialSummary = summarize(measurements.front().milliseconds);
    std::cout << std::fixed << std::setprecision(3)
              << "\nModo          Promedio(ms)  Min(ms)  Max(ms)  Desv(ms)  Speedup  Eficiencia\n";

    csv << "\nmode,average_ms,min_ms,max_ms,stddev_ms,speedup,efficiency_percent\n";
    for (const ModeMeasurements& mode : measurements) {
        const ModeSummary summary = summarize(mode.milliseconds);
        const double speedup = sequentialSummary.average / summary.average;
        const double efficiency = mode.mode == RenderMode::Sequential
                                      ? 100.0
                                      : speedup / static_cast<double>(config.threadCount) * 100.0;

        std::cout << std::left << std::setw(14) << renderModeName(mode.mode) << std::right
                  << std::setw(12) << summary.average << std::setw(9) << summary.minimum
                  << std::setw(9) << summary.maximum << std::setw(10) << summary.standardDeviation
                  << std::setw(9) << speedup << std::setw(11) << efficiency << "%\n";

        csv << renderModeName(mode.mode) << ',' << summary.average << ',' << summary.minimum << ','
            << summary.maximum << ',' << summary.standardDeviation << ',' << speedup << ','
            << efficiency << '\n';
    }

    if (!csv) {
        std::cerr << "Error: no se pudo terminar de escribir el CSV.\n";
        return 1;
    }
    std::cout << "\nResultados guardados en: " << config.benchmarkPath << '\n';
    return 0;
}


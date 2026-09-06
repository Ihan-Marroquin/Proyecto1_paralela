#include "SelfTest.h"

#include "AuroraRenderer.h"
#include "AuroraSimulation.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

class TestRunner {
public:
    void check(bool condition, const std::string& description) {
        if (condition) {
            std::cout << "[OK]    " << description << '\n';
        } else {
            std::cerr << "[FALLO] " << description << '\n';
            ++failures_;
        }
    }

    int failures() const noexcept {
        return failures_;
    }

private:
    int failures_ = 0;
};

bool sourceIsValid(const LightSource& source, int width, int height) {
    const bool finite = std::isfinite(source.x) && std::isfinite(source.y) &&
                        std::isfinite(source.velocityX) && std::isfinite(source.velocityY);
    const bool insideCanvas = source.x >= 0.0F && source.x <= static_cast<float>(width) &&
                              source.y >= 0.0F && source.y <= static_cast<float>(height);
    const bool validColor = source.color.red >= 0.0F && source.color.red <= 1.0F &&
                            source.color.green >= 0.0F && source.color.green <= 1.0F &&
                            source.color.blue >= 0.0F && source.color.blue <= 1.0F;
    return finite && insideCanvas && validColor;
}

} // namespace

int runSelfTests(const AppConfig& config) {
    TestRunner tests;
    const int testSources = std::min(config.sourceCount, 24);

    AuroraSimulation simulation(config.width, config.height, testSources, config.seed);
    for (int step = 0; step < 600; ++step) {
        simulation.update(1.0F / 60.0F);
    }

    bool allSourcesValid = true;
    for (const LightSource& source : simulation.sources()) {
        allSourcesValid = allSourcesValid && sourceIsValid(source, config.width, config.height);
    }
    tests.check(allSourcesValid, "las fuentes permanecen dentro del canvas y con valores finitos");

    AuroraRenderer renderer(config.width, config.height);
    renderer.renderSequential(simulation);
    const std::vector<std::uint32_t> sequentialPixels = renderer.pixels();
    const std::uint64_t sequentialChecksum = renderer.checksum();

    renderer.renderParallel(simulation);
    tests.check(renderer.pixels() == sequentialPixels,
                "el render paralelo coincide pixel por pixel con el secuencial");
    tests.check(renderer.checksum() == sequentialChecksum,
                "el checksum paralelo coincide con la referencia");

    renderer.renderOptimized(simulation);
    tests.check(renderer.pixels() == sequentialPixels,
                "el render optimizado coincide pixel por pixel con el secuencial");
    tests.check(renderer.checksum() == sequentialChecksum,
                "el checksum optimizado coincide con la referencia");

    AuroraSimulation repeatedSimulation(config.width, config.height, testSources, config.seed);
    for (int step = 0; step < 600; ++step) {
        repeatedSimulation.update(1.0F / 60.0F);
    }
    AuroraRenderer repeatedRenderer(config.width, config.height);
    repeatedRenderer.renderSequential(repeatedSimulation);
    tests.check(repeatedRenderer.checksum() == sequentialChecksum,
                "la misma semilla reproduce exactamente la escena");

    if (tests.failures() == 0) {
        std::cout << "\nTodas las pruebas internas terminaron correctamente.\n";
        return 0;
    }
    std::cerr << "\nPruebas fallidas: " << tests.failures() << '\n';
    return 1;
}

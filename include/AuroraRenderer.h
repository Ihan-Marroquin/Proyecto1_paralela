#pragma once

#include "AppConfig.h"
#include "AuroraSimulation.h"

#include <cstdint>
#include <string>
#include <vector>

class AuroraRenderer {
public:
    AuroraRenderer(int width, int height);

    void render(const AuroraSimulation& simulation, RenderMode mode);
    void renderSequential(const AuroraSimulation& simulation);
    void renderParallel(const AuroraSimulation& simulation);

    bool savePpm(const std::string& path, std::string& error) const;
    std::uint64_t checksum() const noexcept;

    const std::vector<std::uint32_t>& pixels() const noexcept;
    int width() const noexcept;
    int height() const noexcept;

private:
    int width_;
    int height_;
    std::vector<std::uint32_t> pixels_;
};

#pragma once

#include <cstdint>
#include <vector>

struct RgbColor {
    float red = 0.0F;
    float green = 0.0F;
    float blue = 0.0F;
};

struct LightSource {
    float x = 0.0F;
    float y = 0.0F;
    float velocityX = 0.0F;
    float velocityY = 0.0F;
    float radius = 80.0F;
    float phase = 0.0F;
    float hue = 0.0F;
    RgbColor color;
};

class AuroraSimulation {
public:
    AuroraSimulation(int width, int height, int sourceCount, std::uint32_t seed);

    void update(float deltaSeconds);
    void reset();

    const std::vector<LightSource>& sources() const noexcept;
    float elapsedSeconds() const noexcept;
    int width() const noexcept;
    int height() const noexcept;

private:
    void initializeSources();

    int width_;
    int height_;
    int sourceCount_;
    std::uint32_t seed_;
    float elapsedSeconds_ = 0.0F;
    std::vector<LightSource> sources_;
};


#include "AuroraSimulation.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace {

constexpr float kPi = 3.14159265358979323846F;

RgbColor hueToRgb(float hue) {
    hue -= std::floor(hue);
    const float scaled = hue * 6.0F;
    const int section = static_cast<int>(scaled);
    const float fraction = scaled - static_cast<float>(section);

    // Saturacion alta y valor completo producen una paleta tipo neon.
    constexpr float minimum = 0.12F;
    const float rising = minimum + (1.0F - minimum) * fraction;
    const float falling = 1.0F - (1.0F - minimum) * fraction;

    switch (section % 6) {
        case 0:
            return {1.0F, rising, minimum};
        case 1:
            return {falling, 1.0F, minimum};
        case 2:
            return {minimum, 1.0F, rising};
        case 3:
            return {minimum, falling, 1.0F};
        case 4:
            return {rising, minimum, 1.0F};
        default:
            return {1.0F, minimum, falling};
    }
}

} // namespace

AuroraSimulation::AuroraSimulation(int width, int height, int sourceCount, std::uint32_t seed)
    : width_(width), height_(height), sourceCount_(sourceCount), seed_(seed) {
    initializeSources();
}

void AuroraSimulation::initializeSources() {
    std::mt19937 generator(seed_);
    std::uniform_real_distribution<float> xDistribution(0.08F * static_cast<float>(width_),
                                                         0.92F * static_cast<float>(width_));
    std::uniform_real_distribution<float> yDistribution(0.10F * static_cast<float>(height_),
                                                         0.90F * static_cast<float>(height_));
    std::uniform_real_distribution<float> angleDistribution(0.0F, 2.0F * kPi);
    std::uniform_real_distribution<float> speedDistribution(24.0F, 72.0F);
    std::uniform_real_distribution<float> radiusDistribution(54.0F, 125.0F);
    std::uniform_real_distribution<float> hueDistribution(0.0F, 1.0F);

    sources_.clear();
    sources_.reserve(static_cast<std::size_t>(sourceCount_));

    for (int index = 0; index < sourceCount_; ++index) {
        const float angle = angleDistribution(generator);
        const float speed = speedDistribution(generator);
        const float hue = std::fmod(hueDistribution(generator) +
                                        static_cast<float>(index) * 0.6180339887F,
                                    1.0F);

        LightSource source;
        source.x = xDistribution(generator);
        source.y = yDistribution(generator);
        source.velocityX = std::cos(angle) * speed;
        source.velocityY = std::sin(angle) * speed;
        source.radius = radiusDistribution(generator);
        source.phase = angleDistribution(generator);
        source.hue = hue;
        source.color = hueToRgb(hue);
        sources_.push_back(source);
    }
}

void AuroraSimulation::update(float deltaSeconds) {
    // Limitar dt evita saltos grandes cuando se mueve o bloquea la ventana.
    const float dt = std::clamp(deltaSeconds, 0.0F, 0.05F);
    elapsedSeconds_ += dt;

    const float centerX = 0.5F * static_cast<float>(width_);
    const float centerY = 0.5F * static_cast<float>(height_);

    for (std::size_t index = 0; index < sources_.size(); ++index) {
        LightSource& source = sources_[index];
        const float direction = source.phase + elapsedSeconds_ * 0.73F +
                                static_cast<float>(index) * 0.19F;

        // Atraccion suave al centro y una fuerza trigonometrica generan orbitas irregulares.
        const float accelerationX = (centerX - source.x) * 0.014F + std::cos(direction) * 11.0F;
        const float accelerationY = (centerY - source.y) * 0.014F + std::sin(direction * 1.17F) * 11.0F;
        source.velocityX += accelerationX * dt;
        source.velocityY += accelerationY * dt;

        const float speedSquared = source.velocityX * source.velocityX +
                                   source.velocityY * source.velocityY;
        constexpr float maximumSpeed = 115.0F;
        if (speedSquared > maximumSpeed * maximumSpeed) {
            const float scale = maximumSpeed / std::sqrt(speedSquared);
            source.velocityX *= scale;
            source.velocityY *= scale;
        }

        source.x += source.velocityX * dt;
        source.y += source.velocityY * dt;

        const float margin = std::max(8.0F, source.radius * 0.18F);
        if (source.x < margin) {
            source.x = margin;
            source.velocityX = std::abs(source.velocityX) * 0.96F;
        } else if (source.x > static_cast<float>(width_) - margin) {
            source.x = static_cast<float>(width_) - margin;
            source.velocityX = -std::abs(source.velocityX) * 0.96F;
        }

        if (source.y < margin) {
            source.y = margin;
            source.velocityY = std::abs(source.velocityY) * 0.96F;
        } else if (source.y > static_cast<float>(height_) - margin) {
            source.y = static_cast<float>(height_) - margin;
            source.velocityY = -std::abs(source.velocityY) * 0.96F;
        }

        source.phase = std::fmod(source.phase + dt * 0.85F, 2.0F * kPi);
        source.color = hueToRgb(source.hue + 0.035F * std::sin(elapsedSeconds_ * 0.22F));
    }
}

void AuroraSimulation::reset() {
    elapsedSeconds_ = 0.0F;
    initializeSources();
}

const std::vector<LightSource>& AuroraSimulation::sources() const noexcept {
    return sources_;
}

float AuroraSimulation::elapsedSeconds() const noexcept {
    return elapsedSeconds_;
}

int AuroraSimulation::width() const noexcept {
    return width_;
}

int AuroraSimulation::height() const noexcept {
    return height_;
}

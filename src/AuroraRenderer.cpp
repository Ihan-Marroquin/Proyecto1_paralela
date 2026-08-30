#include "AuroraRenderer.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace {

float clampUnit(float value) {
    return std::clamp(value, 0.0F, 1.0F);
}

std::uint8_t toByte(float value) {
    return static_cast<std::uint8_t>(255.0F * clampUnit(value) + 0.5F);
}

std::uint32_t packColor(float red, float green, float blue) {
    return (static_cast<std::uint32_t>(toByte(red)) << 16U) |
           (static_cast<std::uint32_t>(toByte(green)) << 8U) |
           static_cast<std::uint32_t>(toByte(blue));
}

std::uint32_t coordinateHash(int x, int y) {
    std::uint32_t value = static_cast<std::uint32_t>(x) * 0x45d9f3bU;
    value ^= static_cast<std::uint32_t>(y) * 0x27d4eb2dU;
    value ^= value >> 16U;
    value *= 0x7feb352dU;
    value ^= value >> 15U;
    return value;
}

std::uint32_t calculatePixel(int x, int y, int width, int height, float time,
                             const std::vector<LightSource>& sources) {
    const float pixelX = static_cast<float>(x);
    const float pixelY = static_cast<float>(y);
    float accumulatedRed = 0.0F;
    float accumulatedGreen = 0.0F;
    float accumulatedBlue = 0.0F;

    for (const LightSource& source : sources) {
        const float deltaX = pixelX - source.x;
        const float deltaY = pixelY - source.y;
        const float distanceSquared = deltaX * deltaX + deltaY * deltaY;
        const float radiusSquared = source.radius * source.radius;
        const float distance = std::sqrt(distanceSquared + 1.0F);
        const float falloff = radiusSquared / (distanceSquared + radiusSquared);
        const float ring = 0.72F + 0.28F *
                                      std::cos(distance * 0.052F - source.phase * 2.4F -
                                               time * 1.35F);
        const float influence = falloff * ring;

        accumulatedRed += influence * source.color.red;
        accumulatedGreen += influence * source.color.green;
        accumulatedBlue += influence * source.color.blue;
    }

    const float sourceScale = 2.15F / std::sqrt(static_cast<float>(sources.size()) + 1.0F);
    accumulatedRed *= sourceScale;
    accumulatedGreen *= sourceScale;
    accumulatedBlue *= sourceScale;

    // Dos ondas senoidales forman la cortina de la aurora.
    const float waveY = static_cast<float>(height) *
                        (0.48F + 0.16F * std::sin(pixelX * 0.010F + time * 0.42F) +
                         0.055F * std::sin(pixelX * 0.027F - time * 0.71F));
    const float distanceToWave = std::abs(pixelY - waveY);
    const float curtain = 1.0F / (1.0F + distanceToWave * 0.034F);
    const float horizontalFade = 0.55F + 0.45F *
                                             std::sin(pixelX / static_cast<float>(width) * 6.2831853F -
                                                      time * 0.18F);

    float red = 0.008F + accumulatedRed * 0.72F + curtain * 0.025F;
    float green = 0.012F + accumulatedGreen * 0.78F + curtain * (0.17F + horizontalFade * 0.08F);
    float blue = 0.035F + accumulatedBlue * 0.88F + curtain * 0.22F;

    // Un hash de coordenadas agrega estrellas estables sin estado ni sincronizacion.
    const std::uint32_t hash = coordinateHash(x, y);
    if ((hash & 0x7ffU) == 0U) {
        const float star = 0.45F + static_cast<float>((hash >> 12U) & 0xffU) / 510.0F;
        red += star;
        green += star;
        blue += star;
    }

    // Mapeo tonal y gamma sencilla para conservar detalle cuando se superponen fuentes.
    red = std::sqrt(red / (1.0F + red));
    green = std::sqrt(green / (1.0F + green));
    blue = std::sqrt(blue / (1.0F + blue));
    return packColor(red, green, blue);
}

} // namespace

AuroraRenderer::AuroraRenderer(int width, int height)
    : width_(width),
      height_(height),
      pixels_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height)) {}

void AuroraRenderer::renderSequential(const AuroraSimulation& simulation) {
    const auto& sources = simulation.sources();
    const float time = simulation.elapsedSeconds();

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const std::size_t pixelIndex = static_cast<std::size_t>(y) *
                                               static_cast<std::size_t>(width_) +
                                           static_cast<std::size_t>(x);
            pixels_[pixelIndex] = calculatePixel(x, y, width_, height_, time, sources);
        }
    }
}

bool AuroraRenderer::savePpm(const std::string& path, std::string& error) const {
    const std::filesystem::path outputPath(path);
    std::error_code directoryError;
    if (outputPath.has_parent_path()) {
        std::filesystem::create_directories(outputPath.parent_path(), directoryError);
        if (directoryError) {
            error = "No se pudo crear el directorio de salida: " + directoryError.message();
            return false;
        }
    }

    std::ofstream output(outputPath, std::ios::binary);
    if (!output) {
        error = "No se pudo abrir la imagen de salida: " + outputPath.string();
        return false;
    }

    output << "P6\n" << width_ << ' ' << height_ << "\n255\n";
    for (const std::uint32_t pixel : pixels_) {
        const char channels[3] = {
            static_cast<char>((pixel >> 16U) & 0xffU),
            static_cast<char>((pixel >> 8U) & 0xffU),
            static_cast<char>(pixel & 0xffU),
        };
        output.write(channels, sizeof(channels));
    }

    if (!output) {
        error = "Ocurrio un error al escribir la imagen.";
        return false;
    }
    return true;
}

std::uint64_t AuroraRenderer::checksum() const noexcept {
    std::uint64_t value = 1469598103934665603ULL;
    for (const std::uint32_t pixel : pixels_) {
        value ^= pixel;
        value *= 1099511628211ULL;
    }
    return value;
}

const std::vector<std::uint32_t>& AuroraRenderer::pixels() const noexcept {
    return pixels_;
}

int AuroraRenderer::width() const noexcept {
    return width_;
}

int AuroraRenderer::height() const noexcept {
    return height_;
}

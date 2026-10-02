#pragma once
#include <cstdint>
#include <string>
#include <glm/glm.hpp>

namespace vox {

enum class DimensionId : uint8_t {
    Overworld = 0,
    Backrooms = 1,
    Count
};

struct DimensionInfo {
    DimensionId id;
    std::string name;
    std::string title;
    std::string subtitle;
    glm::vec3 skyColor;
    glm::vec3 fogColor;
    float fogStart;
    float fogEnd;
    bool hasCeiling;
    bool hasSunlight;
};

inline DimensionInfo getDimensionInfo(DimensionId id) {
    switch (id) {
        case DimensionId::Backrooms:
            return {
                DimensionId::Backrooms,
                "The Backrooms",
                "LEVEL 0",
                "The Yellow Hell",
                glm::vec3(0.72f, 0.65f, 0.32f), // Mono-yellow ambient haze
                glm::vec3(0.52f, 0.46f, 0.20f), // Damp yellow fog
                14.0f,
                32.0f,
                true,   // Indoor enclosed ceiling
                false   // No celestial sun/moon
            };
        case DimensionId::Overworld:
        default:
            return {
                DimensionId::Overworld,
                "Overworld",
                "THE OVERWORLD",
                "The Surface Realm",
                glm::vec3(0.48f, 0.68f, 0.95f),
                glm::vec3(0.55f, 0.72f, 0.95f),
                80.0f,
                160.0f,
                false,
                true
            };
    }
}

} // namespace vox

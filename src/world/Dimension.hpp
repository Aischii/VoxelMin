#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace vox {

// ---------------------------------------------------------------------------
// Dimension Identification & Registry
// Extensible architecture for multi-dimensional worlds (Overworld, Backrooms, etc.)
// ---------------------------------------------------------------------------
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
    bool hasCeiling;       // Enclosed bedrock/roof structure
    bool hasSunlight;      // Celestial day/night light cycle
    float baseAmbientLight;// Ambient minimum lighting (0.0 to 1.0)
    bool allowsMobSpawning;// Surface mob spawning enabled
};

inline const DimensionInfo& getDimensionInfo(DimensionId id) {
    static const DimensionInfo overworldInfo = {
        DimensionId::Overworld,
        "Overworld",
        "THE OVERWORLD",
        "The Surface Realm",
        glm::vec3(0.48f, 0.68f, 0.95f),
        glm::vec3(0.55f, 0.72f, 0.95f),
        80.0f,
        160.0f,
        false,  // Open sky
        true,   // Day/night celestial cycle
        0.0f,   // Standard dynamic sunlight
        true    // Passive & hostile mobs enabled
    };

    static const DimensionInfo backroomsInfo = {
        DimensionId::Backrooms,
        "The Backrooms",
        "LEVEL 0",
        "The Yellow Hell",
        glm::vec3(0.72f, 0.65f, 0.32f), // Mono-yellow ambient haze
        glm::vec3(0.52f, 0.46f, 0.20f), // Damp yellow fog
        14.0f,
        32.0f,
        true,   // Indoor enclosed ceiling
        false,  // No celestial sun/moon
        0.92f,  // Fluorescent ambient diffuse
        false   // No Overworld mobs
    };

    switch (id) {
        case DimensionId::Backrooms:
            return backroomsInfo;
        case DimensionId::Overworld:
        default:
            return overworldInfo;
    }
}

inline const std::vector<DimensionId>& getAllDimensions() {
    static const std::vector<DimensionId> dims = {
        DimensionId::Overworld,
        DimensionId::Backrooms
    };
    return dims;
}

inline bool isValidDimension(DimensionId id) {
    return id < DimensionId::Count;
}

} // namespace vox

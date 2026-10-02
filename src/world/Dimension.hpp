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
        "The Ochre Annex",
        "THE OCHRE ANNEX",
        "The Endless Archives",
        glm::vec3(0.14f, 0.12f, 0.05f), // Sky locked clear color
        glm::vec3(0.18f, 0.16f, 0.07f), // Damp ochre horror fog
        12.0f,
        26.0f,
        true,   // Indoor enclosed ceiling
        false,  // No celestial sun/moon
        0.0f,   // Minimum ambient lighting
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

inline glm::vec3 getDimensionTitleColor(DimensionId id) {
    switch (id) {
        case DimensionId::Backrooms:
            return glm::vec3(0.898f, 0.757f, 0.345f); // #E5C158 Sickly Amber
        case DimensionId::Overworld:
        default:
            return glm::vec3(0.302f, 0.902f, 0.396f); // #4DE665 Pale Jade
    }
}

inline bool isValidDimension(DimensionId id) {
    return id < DimensionId::Count;
}

} // namespace vox

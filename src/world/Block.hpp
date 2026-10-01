#pragma once
#include <cstdint>
#include <glm/glm.hpp>

// ---------------------------------------------------------------------------
// Block types and their visual/physical properties.
//
// Blocks are stored as a single byte (BlockId). All appearance data lives in
// the block definition table below, which maps each block face to a tile in
// the procedural texture atlas.
// ---------------------------------------------------------------------------
namespace vox {

enum class BlockId : uint8_t {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Wood,
    Leaves,
    Sand,
    Bedrock,
    Planks,
    Water,
    Torch,
    Cobblestone,
    CoalOre,
    IronOre,
    GoldOre,
    DiamondOre,
    TallGrass,
    DirtPath,
    WoodX,
    WoodZ,
    TorchWallEast,
    TorchWallWest,
    TorchWallSouth,
    TorchWallNorth,
    Count
};

// Index into the 16x16 procedural atlas (see render/Texture.cpp).
enum class TextureTile : uint8_t {
    GrassTop = 0,
    GrassSide,
    Dirt,
    Stone,
    WoodSide,
    WoodTop,
    Leaves,
    Sand,
    Bedrock,
    Planks,
    Water,
    Torch,
    Cobblestone,
    CoalOre,
    IronOre,
    GoldOre,
    DiamondOre,
    TallGrass,
    DirtPathTop,
    DirtPathSide,
    PigSkin,
    PigFace,
    PigSnout,
    CowSkin,
    CowFace,
    CowHorns,
    PigmanFace,
    PigmanSkin,
    PigmanTorso,
    PigmanHoof,
    PlayerFace,
    PlayerHead,
    PlayerTorso,
    PlayerArm,
    PlayerPants,
    PlayerShoe,
    Count
};

struct BlockDef {
    const char* name;
    TextureTile top;
    TextureTile side;
    TextureTile bottom;
    bool solid; // blocks player movement
};

inline const BlockDef& blockDef(BlockId id) {
    static const BlockDef defs[] = {
        // Air (never rendered, never solid)
        {"Air",              TextureTile::Dirt,         TextureTile::Dirt,         TextureTile::Dirt,         false},
        {"Grass",            TextureTile::GrassTop,     TextureTile::GrassSide,    TextureTile::Dirt,         true},
        {"Dirt",             TextureTile::Dirt,         TextureTile::Dirt,         TextureTile::Dirt,         true},
        {"Stone",            TextureTile::Stone,        TextureTile::Stone,        TextureTile::Stone,        true},
        {"Wood",             TextureTile::WoodTop,      TextureTile::WoodSide,     TextureTile::WoodTop,      true},
        {"Leaves",           TextureTile::Leaves,       TextureTile::Leaves,       TextureTile::Leaves,       true},
        {"Sand",             TextureTile::Sand,         TextureTile::Sand,         TextureTile::Sand,         true},
        {"Bedrock",          TextureTile::Bedrock,      TextureTile::Bedrock,      TextureTile::Bedrock,      true},
        {"Planks",           TextureTile::Planks,       TextureTile::Planks,       TextureTile::Planks,       true},
        {"Water",            TextureTile::Water,        TextureTile::Water,        TextureTile::Water,        false},
        {"Torch",            TextureTile::Torch,        TextureTile::Torch,        TextureTile::Torch,        false},
        {"Cobblestone",      TextureTile::Cobblestone,  TextureTile::Cobblestone,  TextureTile::Cobblestone,  true},
        {"Coal Ore",         TextureTile::CoalOre,      TextureTile::CoalOre,      TextureTile::CoalOre,      true},
        {"Iron Ore",         TextureTile::IronOre,      TextureTile::IronOre,      TextureTile::IronOre,      true},
        {"Gold Ore",         TextureTile::GoldOre,      TextureTile::GoldOre,      TextureTile::GoldOre,      true},
        {"Diamond Ore",      TextureTile::DiamondOre,   TextureTile::DiamondOre,   TextureTile::DiamondOre,   true},
        {"Tall Grass",       TextureTile::TallGrass,    TextureTile::TallGrass,    TextureTile::TallGrass,    false},
        {"Dirt Path",        TextureTile::DirtPathTop,  TextureTile::DirtPathSide, TextureTile::Dirt,         true},
        {"Wood (X)",         TextureTile::WoodSide,     TextureTile::WoodSide,     TextureTile::WoodSide,     true},
        {"Wood (Z)",         TextureTile::WoodSide,     TextureTile::WoodSide,     TextureTile::WoodSide,     true},
        {"Torch (Wall East)",TextureTile::Torch,        TextureTile::Torch,        TextureTile::Torch,        false},
        {"Torch (Wall West)",TextureTile::Torch,        TextureTile::Torch,        TextureTile::Torch,        false},
        {"Torch (Wall South)",TextureTile::Torch,       TextureTile::Torch,        TextureTile::Torch,        false},
        {"Torch (Wall North)",TextureTile::Torch,       TextureTile::Torch,        TextureTile::Torch,        false},
    };
    return defs[static_cast<int>(id)];
}

struct BlockBounds {
    glm::vec3 minOffset{0.0f, 0.0f, 0.0f};
    glm::vec3 maxOffset{1.0f, 1.0f, 1.0f};
};

inline BlockBounds blockBounds(BlockId id) {
    switch (id) {
        case BlockId::Torch:
            // Standing floor torch stick & head (centered 2x2 pixels, 10 pixels tall = 0.625)
            return { {0.375f, 0.0f, 0.375f}, {0.625f, 0.65f, 0.625f} };
        case BlockId::TorchWallEast:
            // Attached to East wall (+X face of block at x+1, slanting toward -X)
            return { {0.45f, 0.15f, 0.35f}, {1.0f, 0.85f, 0.65f} };
        case BlockId::TorchWallWest:
            // Attached to West wall (+X face of block at x-1, slanting toward +X)
            return { {0.0f, 0.15f, 0.35f}, {0.55f, 0.85f, 0.65f} };
        case BlockId::TorchWallSouth:
            // Attached to South wall (+Z face of block at z+1, slanting toward -Z)
            return { {0.35f, 0.15f, 0.45f}, {0.65f, 0.85f, 1.0f} };
        case BlockId::TorchWallNorth:
            // Attached to North wall (+Z face of block at z-1, slanting toward +Z)
            return { {0.35f, 0.15f, 0.0f}, {0.65f, 0.85f, 0.55f} };
        case BlockId::TallGrass:
            // Foliage bounding box
            return { {0.15f, 0.0f, 0.15f}, {0.85f, 0.80f, 0.85f} };
        default:
            return { {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f} };
    }
}

inline bool isAir(BlockId id)         { return id == BlockId::Air; }
inline bool isSolid(BlockId id)       { return blockDef(id).solid; }
inline bool isLiquid(BlockId id)      { return id == BlockId::Water; }
inline bool isTorch(BlockId id) {
    return id == BlockId::Torch ||
           id == BlockId::TorchWallEast ||
           id == BlockId::TorchWallWest ||
           id == BlockId::TorchWallSouth ||
           id == BlockId::TorchWallNorth;
}
inline bool isTransparent(BlockId id) { return id == BlockId::Leaves || id == BlockId::Water || isTorch(id) || id == BlockId::TallGrass; }
inline bool isOpaque(BlockId id)      { return id != BlockId::Air && !isTransparent(id); }
inline bool isLightSource(BlockId id) { return isTorch(id); }
inline bool isBreakable(BlockId id)   { return id != BlockId::Air && id != BlockId::Water && id != BlockId::Bedrock; }
inline bool isPlant(BlockId id)       { return id == BlockId::TallGrass; }

// Flat representative colour, used by the hotbar UI.
inline glm::vec3 blockColor(BlockId id) {
    switch (id) {
        case BlockId::Grass:       return {0.37f, 0.62f, 0.21f};
        case BlockId::Dirt:        return {0.52f, 0.37f, 0.26f};
        case BlockId::Stone:       return {0.50f, 0.50f, 0.50f};
        case BlockId::Wood:
        case BlockId::WoodX:
        case BlockId::WoodZ:       return {0.40f, 0.30f, 0.18f};
        case BlockId::Leaves:      return {0.24f, 0.47f, 0.18f};
        case BlockId::Sand:        return {0.86f, 0.81f, 0.64f};
        case BlockId::Bedrock:     return {0.33f, 0.33f, 0.33f};
        case BlockId::Planks:      return {0.63f, 0.47f, 0.27f};
        case BlockId::Water:       return {0.20f, 0.40f, 0.85f};
        case BlockId::Torch:
        case BlockId::TorchWallEast:
        case BlockId::TorchWallWest:
        case BlockId::TorchWallSouth:
        case BlockId::TorchWallNorth: return {1.00f, 0.80f, 0.20f};
        case BlockId::Cobblestone: return {0.42f, 0.42f, 0.42f};
        case BlockId::CoalOre:     return {0.25f, 0.25f, 0.25f};
        case BlockId::IronOre:     return {0.70f, 0.55f, 0.45f};
        case BlockId::GoldOre:     return {0.95f, 0.85f, 0.25f};
        case BlockId::DiamondOre:  return {0.35f, 0.85f, 0.95f};
        case BlockId::TallGrass:   return {0.33f, 0.68f, 0.20f};
        case BlockId::DirtPath:    return {0.58f, 0.45f, 0.30f};
        default:                   return {1.00f, 0.00f, 1.00f};
    }
}

} // namespace vox

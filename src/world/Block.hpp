#pragma once
#include <cstdint>
#include <cstdlib>
#include <vector>
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
    Stick,
    Coal,
    IronIngot,
    Diamond,
    WoodPickaxe,
    StonePickaxe,
    IronPickaxe,
    DiamondPickaxe,
    WoodAxe,
    StoneAxe,
    IronAxe,
    WoodShovel,
    StoneShovel,
    IronShovel,
    DiamondShovel,
    WoodSword,
    StoneSword,
    IronSword,
    DiamondSword,
    DiamondAxe,
    CraftingTable,
    RawPorkchop,
    CookedPorkchop,
    RawBeef,
    CookedBeef,
    Apple,
    Bread,
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
    PlayerSkin,
    PlayerPants,
    PlayerShoe,
    Stick,
    Coal,
    IronIngot,
    Diamond,
    WoodPickaxe,
    StonePickaxe,
    IronPickaxe,
    DiamondPickaxe,
    WoodAxe,
    StoneAxe,
    IronAxe,
    DiamondAxe,
    WoodShovel,
    StoneShovel,
    IronShovel,
    DiamondShovel,
    WoodSword,
    StoneSword,
    IronSword,
    DiamondSword,
    CraftingTableTop,
    CraftingTableSide,
    CraftingTableFront,
    RawPorkchop,
    CookedPorkchop,
    RawBeef,
    CookedBeef,
    Apple,
    Bread,
    Destroy0,
    Destroy1,
    Destroy2,
    Destroy3,
    Destroy4,
    Destroy5,
    Destroy6,
    Destroy7,
    Destroy8,
    Destroy9,
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
        {"Stick",            TextureTile::Stick,        TextureTile::Stick,        TextureTile::Stick,        false},
        {"Coal",             TextureTile::Coal,         TextureTile::Coal,         TextureTile::Coal,         false},
        {"Iron Ingot",       TextureTile::IronIngot,    TextureTile::IronIngot,    TextureTile::IronIngot,    false},
        {"Diamond",          TextureTile::Diamond,      TextureTile::Diamond,      TextureTile::Diamond,      false},
        {"Wooden Pickaxe",   TextureTile::WoodPickaxe,  TextureTile::WoodPickaxe,  TextureTile::WoodPickaxe,  false},
        {"Stone Pickaxe",    TextureTile::StonePickaxe, TextureTile::StonePickaxe, TextureTile::StonePickaxe, false},
        {"Iron Pickaxe",     TextureTile::IronPickaxe,  TextureTile::IronPickaxe,  TextureTile::IronPickaxe,  false},
        {"Diamond Pickaxe",  TextureTile::DiamondPickaxe, TextureTile::DiamondPickaxe, TextureTile::DiamondPickaxe, false},
        {"Wooden Axe",       TextureTile::WoodAxe,      TextureTile::WoodAxe,      TextureTile::WoodAxe,      false},
        {"Stone Axe",        TextureTile::StoneAxe,     TextureTile::StoneAxe,     TextureTile::StoneAxe,     false},
        {"Iron Axe",         TextureTile::IronAxe,      TextureTile::IronAxe,      TextureTile::IronAxe,      false},
        {"Wooden Shovel",    TextureTile::WoodShovel,   TextureTile::WoodShovel,   TextureTile::WoodShovel,   false},
        {"Stone Shovel",     TextureTile::StoneShovel,  TextureTile::StoneShovel,  TextureTile::StoneShovel,  false},
        {"Iron Shovel",      TextureTile::IronShovel,   TextureTile::IronShovel,   TextureTile::IronShovel,   false},
        {"Diamond Shovel",   TextureTile::DiamondShovel, TextureTile::DiamondShovel, TextureTile::DiamondShovel, false},
        {"Wooden Sword",     TextureTile::WoodSword,    TextureTile::WoodSword,    TextureTile::WoodSword,    false},
        {"Stone Sword",      TextureTile::StoneSword,   TextureTile::StoneSword,   TextureTile::StoneSword,   false},
        {"Iron Sword",       TextureTile::IronSword,    TextureTile::IronSword,    TextureTile::IronSword,    false},
        {"Diamond Sword",    TextureTile::DiamondSword, TextureTile::DiamondSword, TextureTile::DiamondSword, false},
        {"Diamond Axe",      TextureTile::DiamondAxe,   TextureTile::DiamondAxe,   TextureTile::DiamondAxe,   false},
        {"Crafting Table",   TextureTile::CraftingTableTop, TextureTile::CraftingTableSide, TextureTile::Planks, true},
        {"Raw Porkchop",     TextureTile::RawPorkchop,  TextureTile::RawPorkchop,  TextureTile::RawPorkchop,  false},
        {"Cooked Porkchop",  TextureTile::CookedPorkchop, TextureTile::CookedPorkchop, TextureTile::CookedPorkchop, false},
        {"Raw Beef",         TextureTile::RawBeef,      TextureTile::RawBeef,      TextureTile::RawBeef,      false},
        {"Cooked Beef",      TextureTile::CookedBeef,   TextureTile::CookedBeef,   TextureTile::CookedBeef,   false},
        {"Apple",            TextureTile::Apple,        TextureTile::Apple,        TextureTile::Apple,        false},
        {"Bread",            TextureTile::Bread,        TextureTile::Bread,        TextureTile::Bread,        false},
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
            return { {0.375f, 0.0f, 0.375f}, {0.625f, 0.65f, 0.625f} };
        case BlockId::TorchWallEast:
            return { {0.45f, 0.15f, 0.35f}, {1.0f, 0.85f, 0.65f} };
        case BlockId::TorchWallWest:
            return { {0.0f, 0.15f, 0.35f}, {0.55f, 0.85f, 0.65f} };
        case BlockId::TorchWallSouth:
            return { {0.35f, 0.15f, 0.45f}, {0.65f, 0.85f, 1.0f} };
        case BlockId::TorchWallNorth:
            return { {0.35f, 0.15f, 0.0f}, {0.65f, 0.85f, 0.55f} };
        case BlockId::TallGrass:
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
inline bool isOpaque(BlockId id)      { return id != BlockId::Air && !isTransparent(id) && isSolid(id); }
inline bool isLightSource(BlockId id) { return isTorch(id); }
inline bool isBreakable(BlockId id)   { return id != BlockId::Air && id != BlockId::Water && id != BlockId::Bedrock; }
inline bool isPlant(BlockId id)       { return id == BlockId::TallGrass; }

inline bool isPickaxe(BlockId id) {
    return id == BlockId::WoodPickaxe || id == BlockId::StonePickaxe ||
           id == BlockId::IronPickaxe || id == BlockId::DiamondPickaxe;
}

inline bool isAxe(BlockId id) {
    return id == BlockId::WoodAxe || id == BlockId::StoneAxe ||
           id == BlockId::IronAxe || id == BlockId::DiamondAxe;
}

inline bool isShovel(BlockId id) {
    return id == BlockId::WoodShovel || id == BlockId::StoneShovel ||
           id == BlockId::IronShovel || id == BlockId::DiamondShovel;
}

inline bool isSword(BlockId id) {
    return id == BlockId::WoodSword || id == BlockId::StoneSword ||
           id == BlockId::IronSword || id == BlockId::DiamondSword;
}

inline bool isTool(BlockId id) {
    return isPickaxe(id) || isAxe(id) || isShovel(id) || isSword(id);
}

struct FoodProperties {
    int hunger; // Hunger restored (0-100)
    int health; // Instant health restored (0-100)
};

inline bool isFood(BlockId id) {
    return id == BlockId::RawPorkchop || id == BlockId::CookedPorkchop ||
           id == BlockId::RawBeef || id == BlockId::CookedBeef ||
           id == BlockId::Apple || id == BlockId::Bread;
}

inline FoodProperties foodNutrition(BlockId id) {
    switch (id) {
        case BlockId::Apple:          return {15, 5};
        case BlockId::Bread:          return {25, 10};
        case BlockId::RawPorkchop:    return {15, 0};
        case BlockId::CookedPorkchop: return {40, 20};
        case BlockId::RawBeef:        return {15, 0};
        case BlockId::CookedBeef:     return {40, 20};
        default:                      return {0, 0};
    }
}

inline bool isItem(BlockId id) {
    return isTool(id) || isFood(id) || id == BlockId::Stick || id == BlockId::Coal ||
           id == BlockId::IronIngot || id == BlockId::Diamond;
}

inline bool isPlaceable(BlockId id) {
    return id != BlockId::Air && !isItem(id);
}

inline int maxToolDurability(BlockId id) {
    switch (id) {
        case BlockId::WoodPickaxe:
        case BlockId::WoodAxe:
        case BlockId::WoodShovel:
        case BlockId::WoodSword:
            return 60;
        case BlockId::StonePickaxe:
        case BlockId::StoneAxe:
        case BlockId::StoneShovel:
        case BlockId::StoneSword:
            return 132;
        case BlockId::IronPickaxe:
        case BlockId::IronAxe:
        case BlockId::IronShovel:
        case BlockId::IronSword:
            return 250;
        case BlockId::DiamondPickaxe:
        case BlockId::DiamondAxe:
        case BlockId::DiamondShovel:
        case BlockId::DiamondSword:
            return 1561;
        default:
            return 0;
    }
}

inline float toolMiningMultiplier(BlockId tool, BlockId block) {
    if (isPickaxe(tool)) {
        if (block == BlockId::Stone || block == BlockId::Cobblestone ||
            block == BlockId::CoalOre || block == BlockId::IronOre ||
            block == BlockId::GoldOre || block == BlockId::DiamondOre) {
            if (tool == BlockId::DiamondPickaxe) return 8.0f;
            if (tool == BlockId::IronPickaxe)    return 6.0f;
            if (tool == BlockId::StonePickaxe)   return 4.0f;
            return 2.0f;
        }
    } else if (isAxe(tool)) {
        if (block == BlockId::Wood || block == BlockId::WoodX ||
            block == BlockId::WoodZ || block == BlockId::Planks ||
            block == BlockId::CraftingTable) {
            if (tool == BlockId::DiamondAxe) return 8.0f;
            if (tool == BlockId::IronAxe)    return 6.0f;
            if (tool == BlockId::StoneAxe)   return 4.0f;
            return 2.0f;
        }
    } else if (isShovel(tool)) {
        if (block == BlockId::Dirt || block == BlockId::Grass ||
            block == BlockId::Sand || block == BlockId::DirtPath) {
            if (tool == BlockId::DiamondShovel) return 8.0f;
            if (tool == BlockId::IronShovel)    return 6.0f;
            if (tool == BlockId::StoneShovel)   return 4.0f;
            return 2.0f;
        }
    } else if (isSword(tool)) {
        if (block == BlockId::Leaves || block == BlockId::TallGrass) {
            return 15.0f;
        }
    }
    return 1.0f;
}

inline float blockHardness(BlockId id) {
    switch (id) {
        case BlockId::TallGrass:
        case BlockId::Torch:
        case BlockId::TorchWallEast:
        case BlockId::TorchWallWest:
        case BlockId::TorchWallSouth:
        case BlockId::TorchWallNorth:
            return 0.0f;
        case BlockId::Leaves:
            return 0.35f;
        case BlockId::Dirt:
        case BlockId::Grass:
        case BlockId::Sand:
        case BlockId::DirtPath:
            return 0.65f;
        case BlockId::Planks:
            return 1.25f;
        case BlockId::Stone:
            return 1.50f;
        case BlockId::Cobblestone:
        case BlockId::Wood:
        case BlockId::WoodX:
        case BlockId::WoodZ:
        case BlockId::CraftingTable:
            return 2.00f;
        case BlockId::CoalOre:
            return 3.00f;
        case BlockId::IronOre:
        case BlockId::GoldOre:
            return 3.00f;
        case BlockId::DiamondOre:
            return 4.00f;
        case BlockId::Bedrock:
            return -1.0f;
        default:
            return 1.0f;
    }
}

inline bool canHarvestBlock(BlockId tool, BlockId block) {
    if (block == BlockId::DiamondOre || block == BlockId::GoldOre) {
        return tool == BlockId::IronPickaxe || tool == BlockId::DiamondPickaxe;
    }
    if (block == BlockId::IronOre) {
        return tool == BlockId::StonePickaxe || tool == BlockId::IronPickaxe || tool == BlockId::DiamondPickaxe;
    }
    if (block == BlockId::Stone || block == BlockId::Cobblestone || block == BlockId::CoalOre) {
        return isPickaxe(tool);
    }
    return true;
}

inline float getBreakTime(BlockId tool, BlockId block) {
    const float base = blockHardness(block);
    if (base <= 0.0f) return 0.0f;

    float speed = toolMiningMultiplier(tool, block);
    // Mining stone/ores without a pickaxe is severely penalized
    if ((block == BlockId::Stone || block == BlockId::Cobblestone ||
         block == BlockId::CoalOre || block == BlockId::IronOre ||
         block == BlockId::GoldOre || block == BlockId::DiamondOre) && !isPickaxe(tool)) {
        speed = 0.30f;
    }

    return std::max(0.04f, base / speed);
}

inline int attackDamage(BlockId tool) {
    switch (tool) {
        case BlockId::DiamondSword:  return 8;
        case BlockId::IronSword:     return 7;
        case BlockId::StoneSword:    return 6;
        case BlockId::WoodSword:     return 5;
        case BlockId::DiamondAxe:    return 7;
        case BlockId::IronAxe:       return 6;
        case BlockId::StoneAxe:      return 5;
        case BlockId::WoodAxe:       return 4;
        case BlockId::DiamondPickaxe:
        case BlockId::DiamondShovel: return 5;
        case BlockId::IronPickaxe:
        case BlockId::IronShovel:    return 4;
        case BlockId::StonePickaxe:
        case BlockId::StoneShovel:   return 3;
        case BlockId::WoodPickaxe:
        case BlockId::WoodShovel:    return 2;
        default:                     return 1; // Bare fist
    }
}

inline BlockId getDropForBlock(BlockId block) {
    switch (block) {
        case BlockId::Stone:
            return BlockId::Cobblestone;
        case BlockId::Grass:
        case BlockId::DirtPath:
            return BlockId::Dirt;
        case BlockId::CoalOre:
            return BlockId::Coal;
        case BlockId::DiamondOre:
            return BlockId::Diamond;
        case BlockId::Leaves:
            return (std::rand() % 4 == 0) ? BlockId::Stick : BlockId::Air;
        case BlockId::TallGrass:
            return (std::rand() % 5 == 0) ? BlockId::Stick : BlockId::Air;
        case BlockId::WoodX:
        case BlockId::WoodZ:
            return BlockId::Wood;
        case BlockId::TorchWallEast:
        case BlockId::TorchWallWest:
        case BlockId::TorchWallSouth:
        case BlockId::TorchWallNorth:
            return BlockId::Torch;
        default:
            return block;
    }
}

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
        case BlockId::Stick:       return {0.55f, 0.40f, 0.20f};
        case BlockId::Coal:        return {0.18f, 0.18f, 0.18f};
        case BlockId::IronIngot:   return {0.85f, 0.85f, 0.85f};
        case BlockId::Diamond:     return {0.35f, 0.88f, 0.95f};
        case BlockId::WoodPickaxe:
        case BlockId::WoodAxe:
        case BlockId::WoodShovel:
        case BlockId::WoodSword:   return {0.60f, 0.45f, 0.25f};
        case BlockId::StonePickaxe:
        case BlockId::StoneAxe:
        case BlockId::StoneShovel:
        case BlockId::StoneSword:  return {0.55f, 0.55f, 0.55f};
        case BlockId::IronPickaxe:
        case BlockId::IronAxe:
        case BlockId::IronShovel:
        case BlockId::IronSword:   return {0.80f, 0.80f, 0.80f};
        case BlockId::DiamondPickaxe:
        case BlockId::DiamondAxe:
        case BlockId::DiamondShovel:
        case BlockId::DiamondSword: return {0.30f, 0.85f, 0.95f};
        case BlockId::CraftingTable: return {0.65f, 0.50f, 0.30f};
        case BlockId::RawPorkchop:    return {0.95f, 0.60f, 0.62f};
        case BlockId::CookedPorkchop: return {0.75f, 0.44f, 0.22f};
        case BlockId::RawBeef:        return {0.80f, 0.20f, 0.20f};
        case BlockId::CookedBeef:     return {0.55f, 0.28f, 0.16f};
        case BlockId::Apple:          return {0.92f, 0.15f, 0.15f};
        case BlockId::Bread:          return {0.85f, 0.62f, 0.25f};
        default:                   return {1.00f, 0.00f, 1.00f};
    }
}

inline const std::vector<BlockId>& getCreativeCatalog(int tab = 0) {
    static const std::vector<BlockId> allItems = {
        // Blocks & Building
        BlockId::Grass, BlockId::Dirt, BlockId::Stone, BlockId::Cobblestone,
        BlockId::Wood, BlockId::Planks, BlockId::Leaves, BlockId::Sand,
        BlockId::Bedrock, BlockId::Water, BlockId::Torch, BlockId::CraftingTable,
        BlockId::TallGrass, BlockId::DirtPath, BlockId::CoalOre, BlockId::IronOre,
        BlockId::GoldOre, BlockId::DiamondOre,
        // Tools & Combat
        BlockId::WoodPickaxe, BlockId::StonePickaxe, BlockId::IronPickaxe, BlockId::DiamondPickaxe,
        BlockId::WoodAxe, BlockId::StoneAxe, BlockId::IronAxe, BlockId::DiamondAxe,
        BlockId::WoodShovel, BlockId::StoneShovel, BlockId::IronShovel, BlockId::DiamondShovel,
        BlockId::WoodSword, BlockId::StoneSword, BlockId::IronSword, BlockId::DiamondSword,
        // Materials & Food
        BlockId::Stick, BlockId::Coal, BlockId::IronIngot, BlockId::Diamond,
        BlockId::Apple, BlockId::Bread, BlockId::RawPorkchop, BlockId::CookedPorkchop,
        BlockId::RawBeef, BlockId::CookedBeef
    };

    static const std::vector<BlockId> buildingItems = {
        BlockId::Grass, BlockId::Dirt, BlockId::Stone, BlockId::Cobblestone,
        BlockId::Wood, BlockId::Planks, BlockId::Leaves, BlockId::Sand,
        BlockId::Bedrock, BlockId::Water, BlockId::Torch, BlockId::CraftingTable,
        BlockId::TallGrass, BlockId::DirtPath, BlockId::CoalOre, BlockId::IronOre,
        BlockId::GoldOre, BlockId::DiamondOre
    };

    static const std::vector<BlockId> toolItems = {
        BlockId::WoodPickaxe, BlockId::StonePickaxe, BlockId::IronPickaxe, BlockId::DiamondPickaxe,
        BlockId::WoodAxe, BlockId::StoneAxe, BlockId::IronAxe, BlockId::DiamondAxe,
        BlockId::WoodShovel, BlockId::StoneShovel, BlockId::IronShovel, BlockId::DiamondShovel,
        BlockId::WoodSword, BlockId::StoneSword, BlockId::IronSword, BlockId::DiamondSword
    };

    static const std::vector<BlockId> foodMatItems = {
        BlockId::Stick, BlockId::Coal, BlockId::IronIngot, BlockId::Diamond,
        BlockId::Apple, BlockId::Bread, BlockId::RawPorkchop, BlockId::CookedPorkchop,
        BlockId::RawBeef, BlockId::CookedBeef
    };

    switch (tab) {
        case 1: return buildingItems;
        case 2: return toolItems;
        case 3: return foodMatItems;
        case 0:
        default: return allItems;
    }
}

} // namespace vox

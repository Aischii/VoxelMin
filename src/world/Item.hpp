#pragma once
#include "world/Block.hpp"

#include <algorithm>
#include <cstdint>

namespace vox {

// ---------------------------------------------------------------------------
// ItemSlot: Storage container unit for inventory, hotbar, and recipes.
// ---------------------------------------------------------------------------
struct ItemSlot {
    BlockId id = BlockId::Air;
    int count = 0;
    int durability = 0;

    bool empty() const { return id == BlockId::Air || count <= 0; }
    void clear() { id = BlockId::Air; count = 0; durability = 0; }
};

// ---------------------------------------------------------------------------
// Item Properties & Classification
// ---------------------------------------------------------------------------
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
           id == BlockId::Apple || id == BlockId::Bread || id == BlockId::AlmondWater;
}

inline FoodProperties foodNutrition(BlockId id) {
    switch (id) {
        case BlockId::Apple:          return {15, 5};
        case BlockId::Bread:          return {25, 10};
        case BlockId::RawPorkchop:    return {15, 0};
        case BlockId::CookedPorkchop: return {40, 20};
        case BlockId::RawBeef:        return {15, 0};
        case BlockId::CookedBeef:     return {40, 20};
        case BlockId::AlmondWater:    return {45, 35};
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
        case BlockId::AlmondWater:
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

inline BlockId getDropForBlock(BlockId id) {
    switch (id) {
        case BlockId::Grass:      return BlockId::Dirt;
        case BlockId::Stone:      return BlockId::Cobblestone;
        case BlockId::CoalOre:    return BlockId::Coal;
        case BlockId::DiamondOre: return BlockId::Diamond;
        case BlockId::WoodX:
        case BlockId::WoodZ:      return BlockId::Wood;
        case BlockId::TorchWallEast:
        case BlockId::TorchWallWest:
        case BlockId::TorchWallSouth:
        case BlockId::TorchWallNorth: return BlockId::Torch;
        case BlockId::TallGrass:  return BlockId::Air;
        default:                  return id;
    }
}

} // namespace vox

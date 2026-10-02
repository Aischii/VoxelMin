#pragma once
#include "world/Block.hpp"
#include <array>
#include <initializer_list>
#include <string>
#include <vector>

namespace vox {

struct RecipeIng {
    BlockId id = BlockId::Air;
    int count = 1;
};

struct ConsoleRecipeDef {
    const char* name = "";
    BlockId outputId = BlockId::Air;
    int outputCount = 1;
    bool tableOnly = false;
    int ingredientCount = 0;
    std::vector<RecipeIng> ingredients;
    std::array<BlockId, 9> gridPreview{};

    ConsoleRecipeDef(const char* n, BlockId outId, int outCount, bool table, int ingCount,
                     std::initializer_list<RecipeIng> ings,
                     std::initializer_list<BlockId> grid)
        : name(n), outputId(outId), outputCount(outCount), tableOnly(table),
          ingredientCount(ingCount), ingredients(ings) {
        int idx = 0;
        for (BlockId b : grid) {
            if (idx < 9) gridPreview[idx++] = b;
        }
    }
};

struct RecipeCategory {
    const char* name = "";
    TextureTile tabIcon = TextureTile::GrassTop;
    std::vector<ConsoleRecipeDef> recipes;

    RecipeCategory(const char* n, TextureTile icon, std::initializer_list<ConsoleRecipeDef> recs)
        : name(n), tabIcon(icon), recipes(recs) {}
};

inline const std::vector<RecipeCategory>& getConsoleRecipeCategories(bool hasCraftingTable = false) {
    static const std::vector<RecipeCategory> portableCategories = {
        {
            "Structures",
            TextureTile::Planks,
            {
                {
                    "Oak Planks", BlockId::Planks, 4, false, 1,
                    { {BlockId::Wood, 1} },
                    { BlockId::Wood, BlockId::Air, BlockId::Air,
                      BlockId::Air,  BlockId::Air, BlockId::Air,
                      BlockId::Air,  BlockId::Air, BlockId::Air }
                },
                {
                    "Crafting Table", BlockId::CraftingTable, 1, false, 1,
                    { {BlockId::Planks, 4} },
                    { BlockId::Planks, BlockId::Planks, BlockId::Air,
                      BlockId::Planks, BlockId::Planks, BlockId::Air,
                      BlockId::Air,    BlockId::Air,    BlockId::Air }
                },
                {
                    "Cobblestone", BlockId::Cobblestone, 1, false, 1,
                    { {BlockId::Stone, 1} },
                    { BlockId::Stone, BlockId::Air, BlockId::Air,
                      BlockId::Air,   BlockId::Air, BlockId::Air,
                      BlockId::Air,   BlockId::Air, BlockId::Air }
                },
                {
                    "Dirt Path", BlockId::DirtPath, 4, false, 1,
                    { {BlockId::Dirt, 4} },
                    { BlockId::Dirt, BlockId::Dirt, BlockId::Air,
                      BlockId::Dirt, BlockId::Dirt, BlockId::Air,
                      BlockId::Air,  BlockId::Air,  BlockId::Air }
                }
            }
        },
        {
            "Tools & Weapons",
            TextureTile::IronSword,
            {
                {
                    "Wooden Sword", BlockId::WoodSword, 1, false, 2,
                    { {BlockId::Planks, 2}, {BlockId::Stick, 1} },
                    { BlockId::Planks, BlockId::Air, BlockId::Air,
                      BlockId::Planks, BlockId::Air, BlockId::Air,
                      BlockId::Stick,  BlockId::Air, BlockId::Air }
                },
                {
                    "Stone Sword", BlockId::StoneSword, 1, false, 2,
                    { {BlockId::Cobblestone, 2}, {BlockId::Stick, 1} },
                    { BlockId::Cobblestone, BlockId::Air, BlockId::Air,
                      BlockId::Cobblestone, BlockId::Air, BlockId::Air,
                      BlockId::Stick,       BlockId::Air, BlockId::Air }
                },
                {
                    "Iron Sword", BlockId::IronSword, 1, false, 2,
                    { {BlockId::IronIngot, 2}, {BlockId::Stick, 1} },
                    { BlockId::IronIngot, BlockId::Air, BlockId::Air,
                      BlockId::IronIngot, BlockId::Air, BlockId::Air,
                      BlockId::Stick,     BlockId::Air, BlockId::Air }
                },
                {
                    "Diamond Sword", BlockId::DiamondSword, 1, false, 2,
                    { {BlockId::Diamond, 2}, {BlockId::Stick, 1} },
                    { BlockId::Diamond, BlockId::Air, BlockId::Air,
                      BlockId::Diamond, BlockId::Air, BlockId::Air,
                      BlockId::Stick,   BlockId::Air, BlockId::Air }
                },
                {
                    "Wooden Pickaxe", BlockId::WoodPickaxe, 1, true, 2,
                    { {BlockId::Planks, 3}, {BlockId::Stick, 2} },
                    { BlockId::Planks, BlockId::Planks, BlockId::Planks,
                      BlockId::Air,    BlockId::Stick,  BlockId::Air,
                      BlockId::Air,    BlockId::Stick,  BlockId::Air }
                },
                {
                    "Stone Pickaxe", BlockId::StonePickaxe, 1, true, 2,
                    { {BlockId::Cobblestone, 3}, {BlockId::Stick, 2} },
                    { BlockId::Cobblestone, BlockId::Cobblestone, BlockId::Cobblestone,
                      BlockId::Air,         BlockId::Stick,       BlockId::Air,
                      BlockId::Air,         BlockId::Stick,       BlockId::Air }
                },
                {
                    "Iron Pickaxe", BlockId::IronPickaxe, 1, true, 2,
                    { {BlockId::IronIngot, 3}, {BlockId::Stick, 2} },
                    { BlockId::IronIngot, BlockId::IronIngot, BlockId::IronIngot,
                      BlockId::Air,       BlockId::Stick,     BlockId::Air,
                      BlockId::Air,       BlockId::Stick,     BlockId::Air }
                },
                {
                    "Diamond Pickaxe", BlockId::DiamondPickaxe, 1, true, 2,
                    { {BlockId::Diamond, 3}, {BlockId::Stick, 2} },
                    { BlockId::Diamond, BlockId::Diamond, BlockId::Diamond,
                      BlockId::Air,     BlockId::Stick,   BlockId::Air,
                      BlockId::Air,     BlockId::Stick,   BlockId::Air }
                },
                {
                    "Wooden Axe", BlockId::WoodAxe, 1, true, 2,
                    { {BlockId::Planks, 3}, {BlockId::Stick, 2} },
                    { BlockId::Planks, BlockId::Planks, BlockId::Air,
                      BlockId::Planks, BlockId::Stick,  BlockId::Air,
                      BlockId::Air,    BlockId::Stick,  BlockId::Air }
                },
                {
                    "Stone Axe", BlockId::StoneAxe, 1, true, 2,
                    { {BlockId::Cobblestone, 3}, {BlockId::Stick, 2} },
                    { BlockId::Cobblestone, BlockId::Cobblestone, BlockId::Air,
                      BlockId::Cobblestone, BlockId::Stick,       BlockId::Air,
                      BlockId::Air,         BlockId::Stick,       BlockId::Air }
                },
                {
                    "Iron Axe", BlockId::IronAxe, 1, true, 2,
                    { {BlockId::IronIngot, 3}, {BlockId::Stick, 2} },
                    { BlockId::IronIngot, BlockId::IronIngot, BlockId::Air,
                      BlockId::IronIngot, BlockId::Stick,     BlockId::Air,
                      BlockId::Air,       BlockId::Stick,     BlockId::Air }
                },
                {
                    "Diamond Axe", BlockId::DiamondAxe, 1, true, 2,
                    { {BlockId::Diamond, 3}, {BlockId::Stick, 2} },
                    { BlockId::Diamond, BlockId::Diamond, BlockId::Air,
                      BlockId::Diamond, BlockId::Stick,   BlockId::Air,
                      BlockId::Air,     BlockId::Stick,   BlockId::Air }
                },
                {
                    "Wooden Shovel", BlockId::WoodShovel, 1, false, 2,
                    { {BlockId::Planks, 1}, {BlockId::Stick, 2} },
                    { BlockId::Planks, BlockId::Air, BlockId::Air,
                      BlockId::Stick,  BlockId::Air, BlockId::Air,
                      BlockId::Stick,  BlockId::Air, BlockId::Air }
                },
                {
                    "Stone Shovel", BlockId::StoneShovel, 1, false, 2,
                    { {BlockId::Cobblestone, 1}, {BlockId::Stick, 2} },
                    { BlockId::Cobblestone, BlockId::Cobblestone, BlockId::Air,
                      BlockId::Stick,       BlockId::Air,         BlockId::Air,
                      BlockId::Stick,       BlockId::Air,         BlockId::Air }
                },
                {
                    "Iron Shovel", BlockId::IronShovel, 1, false, 2,
                    { {BlockId::IronIngot, 1}, {BlockId::Stick, 2} },
                    { BlockId::IronIngot, BlockId::Air, BlockId::Air,
                      BlockId::Stick,     BlockId::Air, BlockId::Air,
                      BlockId::Stick,     BlockId::Air, BlockId::Air }
                },
                {
                    "Diamond Shovel", BlockId::DiamondShovel, 1, false, 2,
                    { {BlockId::Diamond, 1}, {BlockId::Stick, 2} },
                    { BlockId::Diamond, BlockId::Air, BlockId::Air,
                      BlockId::Stick,   BlockId::Air, BlockId::Air,
                      BlockId::Stick,   BlockId::Air, BlockId::Air }
                }
            }
        },
        {
            "Food & Essentials",
            TextureTile::Apple,
            {
                {
                    "Sticks", BlockId::Stick, 4, false, 1,
                    { {BlockId::Planks, 2} },
                    { BlockId::Planks, BlockId::Air, BlockId::Air,
                      BlockId::Planks, BlockId::Air, BlockId::Air,
                      BlockId::Air,    BlockId::Air, BlockId::Air }
                },
                {
                    "Torches", BlockId::Torch, 4, false, 2,
                    { {BlockId::Coal, 1}, {BlockId::Stick, 1} },
                    { BlockId::Coal,  BlockId::Air, BlockId::Air,
                      BlockId::Stick, BlockId::Air, BlockId::Air,
                      BlockId::Air,   BlockId::Air, BlockId::Air }
                },
                {
                    "Cooked Porkchop", BlockId::CookedPorkchop, 1, false, 2,
                    { {BlockId::RawPorkchop, 1}, {BlockId::Coal, 1} },
                    { BlockId::RawPorkchop, BlockId::Air, BlockId::Air,
                      BlockId::Coal,        BlockId::Air, BlockId::Air,
                      BlockId::Air,         BlockId::Air, BlockId::Air }
                },
                {
                    "Cooked Beef", BlockId::CookedBeef, 1, false, 2,
                    { {BlockId::RawBeef, 1}, {BlockId::Coal, 1} },
                    { BlockId::RawBeef, BlockId::Air, BlockId::Air,
                      BlockId::Coal,    BlockId::Air, BlockId::Air,
                      BlockId::Air,     BlockId::Air, BlockId::Air }
                },
                {
                    "Apples", BlockId::Apple, 2, false, 1,
                    { {BlockId::Leaves, 4} },
                    { BlockId::Leaves, BlockId::Leaves, BlockId::Air,
                      BlockId::Leaves, BlockId::Leaves, BlockId::Air,
                      BlockId::Air,    BlockId::Air,    BlockId::Air }
                },
                {
                    "Bread", BlockId::Bread, 1, false, 1,
                    { {BlockId::Leaves, 3} },
                    { BlockId::Leaves, BlockId::Leaves, BlockId::Leaves,
                      BlockId::Air,    BlockId::Air,    BlockId::Air,
                      BlockId::Air,    BlockId::Air,    BlockId::Air }
                }
            }
        },
        {
            "Mechanisms",
            TextureTile::CraftingTableTop,
            {
                {
                    "Crafting Table", BlockId::CraftingTable, 1, false, 1,
                    { {BlockId::Planks, 4} },
                    { BlockId::Planks, BlockId::Planks, BlockId::Air,
                      BlockId::Planks, BlockId::Planks, BlockId::Air,
                      BlockId::Air,    BlockId::Air,    BlockId::Air }
                },
                {
                    "Torches", BlockId::Torch, 4, false, 2,
                    { {BlockId::Coal, 1}, {BlockId::Stick, 1} },
                    { BlockId::Coal,  BlockId::Air, BlockId::Air,
                      BlockId::Stick, BlockId::Air, BlockId::Air,
                      BlockId::Air,   BlockId::Air, BlockId::Air }
                }
            }
        },
        {
            "Transport",
            TextureTile::DirtPathTop,
            {
                {
                    "Dirt Path", BlockId::DirtPath, 4, false, 1,
                    { {BlockId::Dirt, 4} },
                    { BlockId::Dirt, BlockId::Dirt, BlockId::Air,
                      BlockId::Dirt, BlockId::Dirt, BlockId::Air,
                      BlockId::Air,  BlockId::Air,  BlockId::Air }
                }
            }
        },
        {
            "Decorations",
            TextureTile::TallGrass,
            {
                {
                    "Tall Grass", BlockId::TallGrass, 2, false, 1,
                    { {BlockId::Leaves, 2} },
                    { BlockId::Leaves, BlockId::Air, BlockId::Air,
                      BlockId::Leaves, BlockId::Air, BlockId::Air,
                      BlockId::Air,    BlockId::Air,    BlockId::Air }
                }
            }
        }
    };

    static const std::vector<RecipeCategory> tableCategories = {
        {
            "Structures",
            TextureTile::Planks,
            {
                {
                    "Oak Planks", BlockId::Planks, 4, false, 1,
                    { {BlockId::Wood, 1} },
                    { BlockId::Wood, BlockId::Air, BlockId::Air,
                      BlockId::Air,  BlockId::Air, BlockId::Air,
                      BlockId::Air,  BlockId::Air, BlockId::Air }
                },
                {
                    "Crafting Table", BlockId::CraftingTable, 1, false, 1,
                    { {BlockId::Planks, 4} },
                    { BlockId::Planks, BlockId::Planks, BlockId::Air,
                      BlockId::Planks, BlockId::Planks, BlockId::Air,
                      BlockId::Air,    BlockId::Air,    BlockId::Air }
                },
                {
                    "Cobblestone", BlockId::Cobblestone, 1, false, 1,
                    { {BlockId::Stone, 1} },
                    { BlockId::Stone, BlockId::Air, BlockId::Air,
                      BlockId::Air,   BlockId::Air, BlockId::Air,
                      BlockId::Air,   BlockId::Air, BlockId::Air }
                },
                {
                    "Dirt Path", BlockId::DirtPath, 4, false, 1,
                    { {BlockId::Dirt, 4} },
                    { BlockId::Dirt, BlockId::Dirt, BlockId::Air,
                      BlockId::Dirt, BlockId::Dirt, BlockId::Air,
                      BlockId::Air,  BlockId::Air,  BlockId::Air }
                }
            }
        },
        {
            "Tools & Weapons",
            TextureTile::IronSword,
            {
                {
                    "Wooden Sword", BlockId::WoodSword, 1, false, 2,
                    { {BlockId::Planks, 2}, {BlockId::Stick, 1} },
                    { BlockId::Planks, BlockId::Air, BlockId::Air,
                      BlockId::Planks, BlockId::Air, BlockId::Air,
                      BlockId::Stick,  BlockId::Air, BlockId::Air }
                },
                {
                    "Stone Sword", BlockId::StoneSword, 1, false, 2,
                    { {BlockId::Cobblestone, 2}, {BlockId::Stick, 1} },
                    { BlockId::Cobblestone, BlockId::Air, BlockId::Air,
                      BlockId::Cobblestone, BlockId::Air, BlockId::Air,
                      BlockId::Stick,       BlockId::Air, BlockId::Air }
                },
                {
                    "Iron Sword", BlockId::IronSword, 1, false, 2,
                    { {BlockId::IronIngot, 2}, {BlockId::Stick, 1} },
                    { BlockId::IronIngot, BlockId::Air, BlockId::Air,
                      BlockId::IronIngot, BlockId::Air, BlockId::Air,
                      BlockId::Stick,     BlockId::Air, BlockId::Air }
                },
                {
                    "Diamond Sword", BlockId::DiamondSword, 1, false, 2,
                    { {BlockId::Diamond, 2}, {BlockId::Stick, 1} },
                    { BlockId::Diamond, BlockId::Air, BlockId::Air,
                      BlockId::Diamond, BlockId::Air, BlockId::Air,
                      BlockId::Stick,   BlockId::Air, BlockId::Air }
                },
                {
                    "Wooden Pickaxe", BlockId::WoodPickaxe, 1, true, 2,
                    { {BlockId::Planks, 3}, {BlockId::Stick, 2} },
                    { BlockId::Planks, BlockId::Planks, BlockId::Planks,
                      BlockId::Air,    BlockId::Stick,  BlockId::Air,
                      BlockId::Air,    BlockId::Stick,  BlockId::Air }
                },
                {
                    "Stone Pickaxe", BlockId::StonePickaxe, 1, true, 2,
                    { {BlockId::Cobblestone, 3}, {BlockId::Stick, 2} },
                    { BlockId::Cobblestone, BlockId::Cobblestone, BlockId::Cobblestone,
                      BlockId::Air,         BlockId::Stick,       BlockId::Air,
                      BlockId::Air,         BlockId::Stick,       BlockId::Air }
                },
                {
                    "Iron Pickaxe", BlockId::IronPickaxe, 1, true, 2,
                    { {BlockId::IronIngot, 3}, {BlockId::Stick, 2} },
                    { BlockId::IronIngot, BlockId::IronIngot, BlockId::IronIngot,
                      BlockId::Air,       BlockId::Stick,     BlockId::Air,
                      BlockId::Air,       BlockId::Stick,     BlockId::Air }
                },
                {
                    "Diamond Pickaxe", BlockId::DiamondPickaxe, 1, true, 2,
                    { {BlockId::Diamond, 3}, {BlockId::Stick, 2} },
                    { BlockId::Diamond, BlockId::Diamond, BlockId::Diamond,
                      BlockId::Air,     BlockId::Stick,   BlockId::Air,
                      BlockId::Air,     BlockId::Stick,   BlockId::Air }
                },
                {
                    "Wooden Axe", BlockId::WoodAxe, 1, true, 2,
                    { {BlockId::Planks, 3}, {BlockId::Stick, 2} },
                    { BlockId::Planks, BlockId::Planks, BlockId::Air,
                      BlockId::Planks, BlockId::Stick,  BlockId::Air,
                      BlockId::Air,    BlockId::Stick,  BlockId::Air }
                },
                {
                    "Stone Axe", BlockId::StoneAxe, 1, true, 2,
                    { {BlockId::Cobblestone, 3}, {BlockId::Stick, 2} },
                    { BlockId::Cobblestone, BlockId::Cobblestone, BlockId::Air,
                      BlockId::Cobblestone, BlockId::Stick,       BlockId::Air,
                      BlockId::Air,         BlockId::Stick,       BlockId::Air }
                },
                {
                    "Iron Axe", BlockId::IronAxe, 1, true, 2,
                    { {BlockId::IronIngot, 3}, {BlockId::Stick, 2} },
                    { BlockId::IronIngot, BlockId::IronIngot, BlockId::Air,
                      BlockId::IronIngot, BlockId::Stick,     BlockId::Air,
                      BlockId::Air,       BlockId::Stick,     BlockId::Air }
                },
                {
                    "Diamond Axe", BlockId::DiamondAxe, 1, true, 2,
                    { {BlockId::Diamond, 3}, {BlockId::Stick, 2} },
                    { BlockId::Diamond, BlockId::Diamond, BlockId::Air,
                      BlockId::Diamond, BlockId::Stick,   BlockId::Air,
                      BlockId::Air,     BlockId::Stick,   BlockId::Air }
                },
                {
                    "Wooden Shovel", BlockId::WoodShovel, 1, false, 2,
                    { {BlockId::Planks, 1}, {BlockId::Stick, 2} },
                    { BlockId::Planks, BlockId::Air, BlockId::Air,
                      BlockId::Stick,  BlockId::Air, BlockId::Air,
                      BlockId::Stick,  BlockId::Air, BlockId::Air }
                },
                {
                    "Stone Shovel", BlockId::StoneShovel, 1, false, 2,
                    { {BlockId::Cobblestone, 1}, {BlockId::Stick, 2} },
                    { BlockId::Cobblestone, BlockId::Air, BlockId::Air,
                      BlockId::Stick,       BlockId::Air, BlockId::Air,
                      BlockId::Stick,       BlockId::Air, BlockId::Air }
                },
                {
                    "Iron Shovel", BlockId::IronShovel, 1, false, 2,
                    { {BlockId::IronIngot, 1}, {BlockId::Stick, 2} },
                    { BlockId::IronIngot, BlockId::Air, BlockId::Air,
                      BlockId::Stick,     BlockId::Air, BlockId::Air,
                      BlockId::Stick,     BlockId::Air, BlockId::Air }
                },
                {
                    "Diamond Shovel", BlockId::DiamondShovel, 1, false, 2,
                    { {BlockId::Diamond, 1}, {BlockId::Stick, 2} },
                    { BlockId::Diamond, BlockId::Air, BlockId::Air,
                      BlockId::Stick,   BlockId::Air, BlockId::Air,
                      BlockId::Stick,   BlockId::Air, BlockId::Air }
                }
            }
        },
        {
            "Food",
            TextureTile::Apple,
            {
                {
                    "Cooked Porkchop", BlockId::CookedPorkchop, 1, false, 2,
                    { {BlockId::RawPorkchop, 1}, {BlockId::Coal, 1} },
                    { BlockId::RawPorkchop, BlockId::Air, BlockId::Air,
                      BlockId::Coal,        BlockId::Air, BlockId::Air,
                      BlockId::Air,         BlockId::Air, BlockId::Air }
                },
                {
                    "Cooked Beef", BlockId::CookedBeef, 1, false, 2,
                    { {BlockId::RawBeef, 1}, {BlockId::Coal, 1} },
                    { BlockId::RawBeef, BlockId::Air, BlockId::Air,
                      BlockId::Coal,    BlockId::Air, BlockId::Air,
                      BlockId::Air,     BlockId::Air, BlockId::Air }
                },
                {
                    "Apples", BlockId::Apple, 2, false, 1,
                    { {BlockId::Leaves, 4} },
                    { BlockId::Leaves, BlockId::Leaves, BlockId::Air,
                      BlockId::Leaves, BlockId::Leaves, BlockId::Air,
                      BlockId::Air,    BlockId::Air,    BlockId::Air }
                },
                {
                    "Bread", BlockId::Bread, 1, false, 1,
                    { {BlockId::Leaves, 3} },
                    { BlockId::Leaves, BlockId::Leaves, BlockId::Leaves,
                      BlockId::Air,    BlockId::Air,    BlockId::Air,
                      BlockId::Air,    BlockId::Air,    BlockId::Air }
                }
            }
        },
        {
            "Armor",
            TextureTile::IronIngot,
            {
                {
                    "Iron Ingot Block", BlockId::IronOre, 1, true, 1,
                    { {BlockId::IronIngot, 4} },
                    { BlockId::IronIngot, BlockId::IronIngot, BlockId::Air,
                      BlockId::IronIngot, BlockId::IronIngot, BlockId::Air,
                      BlockId::Air,       BlockId::Air,       BlockId::Air }
                },
                {
                    "Diamond Ore Block", BlockId::DiamondOre, 1, true, 1,
                    { {BlockId::Diamond, 4} },
                    { BlockId::Diamond, BlockId::Diamond, BlockId::Air,
                      BlockId::Diamond, BlockId::Diamond, BlockId::Air,
                      BlockId::Air,     BlockId::Air,     BlockId::Air }
                }
            }
        },
        {
            "Mechanisms",
            TextureTile::CraftingTableTop,
            {
                {
                    "Crafting Table", BlockId::CraftingTable, 1, false, 1,
                    { {BlockId::Planks, 4} },
                    { BlockId::Planks, BlockId::Planks, BlockId::Air,
                      BlockId::Planks, BlockId::Planks, BlockId::Air,
                      BlockId::Air,    BlockId::Air,    BlockId::Air }
                },
                {
                    "Torches", BlockId::Torch, 4, false, 2,
                    { {BlockId::Coal, 1}, {BlockId::Stick, 1} },
                    { BlockId::Coal,  BlockId::Air, BlockId::Air,
                      BlockId::Stick, BlockId::Air, BlockId::Air,
                      BlockId::Air,   BlockId::Air, BlockId::Air }
                },
                {
                    "Sticks", BlockId::Stick, 4, false, 1,
                    { {BlockId::Planks, 2} },
                    { BlockId::Planks, BlockId::Air, BlockId::Air,
                      BlockId::Planks, BlockId::Air, BlockId::Air,
                      BlockId::Air,    BlockId::Air,    BlockId::Air }
                }
            }
        },
        {
            "Transport",
            TextureTile::DirtPathTop,
            {
                {
                    "Dirt Path", BlockId::DirtPath, 4, false, 1,
                    { {BlockId::Dirt, 4} },
                    { BlockId::Dirt, BlockId::Dirt, BlockId::Air,
                      BlockId::Dirt, BlockId::Dirt, BlockId::Air,
                      BlockId::Air,  BlockId::Air,  BlockId::Air }
                }
            }
        },
        {
            "Decorations",
            TextureTile::TallGrass,
            {
                {
                    "Tall Grass", BlockId::TallGrass, 2, false, 1,
                    { {BlockId::Leaves, 2} },
                    { BlockId::Leaves, BlockId::Air, BlockId::Air,
                      BlockId::Leaves, BlockId::Air, BlockId::Air,
                      BlockId::Air,    BlockId::Air,    BlockId::Air }
                }
            }
        }
    };

    return hasCraftingTable ? tableCategories : portableCategories;
}

} // namespace vox

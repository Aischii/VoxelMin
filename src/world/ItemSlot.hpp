#pragma once
#include "world/Block.hpp"

namespace vox {

// Represents an inventory slot holding an item stack and optional durability.
struct ItemSlot {
    BlockId id = BlockId::Air;
    int count = 0;
    int durability = 0;

    ItemSlot() = default;
    ItemSlot(BlockId blockId, int stackCount = 1, int initialDurability = 0)
        : id(blockId), count(stackCount), durability(initialDurability) {
        if (id != BlockId::Air && isTool(id) && durability <= 0) {
            durability = maxToolDurability(id);
        }
    }

    bool empty() const { return id == BlockId::Air || count <= 0; }

    void clear() {
        id = BlockId::Air;
        count = 0;
        durability = 0;
    }
};

} // namespace vox

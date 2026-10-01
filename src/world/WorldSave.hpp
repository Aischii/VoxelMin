#pragma once
#include "world/Block.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace vox {

class World;
class Player;

struct WorldMetadata {
    std::string name;
    std::string path;
    uint32_t seed = 1337;
};

class WorldSave {
public:
    static std::string getSavesDirectory();
    static std::string sanitizeWorldName(const std::string& name);
    static std::string getWorldPath(const std::string& worldName);
    static std::vector<WorldMetadata> listSavedWorlds();
    static bool peekWorld(const std::string& path, std::string& outName, uint32_t& outSeed);

    static bool saveGame(const std::string& path, const std::string& worldName, uint32_t seed,
                         const World& world, const Player& player,
                         int selectedSlot, const BlockId* hotbar, const BlockId* inventory);

    static bool loadGame(const std::string& path, std::string& outWorldName, uint32_t& outSeed,
                         World& world, Player& player,
                         int& selectedSlot, BlockId* hotbar, BlockId* inventory);

    static bool saveExists(const std::string& path);
    static bool deleteWorld(const std::string& path);
};

} // namespace vox

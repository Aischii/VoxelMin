#include "world/WorldSave.hpp"
#include "core/Config.hpp"
#include "core/Log.hpp"
#include "player/Player.hpp"
#include "world/World.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

namespace vox {
namespace {

constexpr uint32_t SAVE_MAGIC = 0x53584F56; // 'VOXS'
constexpr uint32_t SAVE_VERSION_1 = 1;
constexpr uint32_t SAVE_VERSION_2 = 2;

} // namespace

std::string WorldSave::getSavesDirectory() {
    std::error_code ec;
    // Check if running from build/bin (i.e. ../../assets or ../../CMakeLists.txt exists)
    if (std::filesystem::exists("../../CMakeLists.txt", ec) || std::filesystem::exists("../../assets", ec)) {
        std::filesystem::create_directories("../../saves", ec);
        return "../../saves";
    }
    if (std::filesystem::exists("../CMakeLists.txt", ec) || std::filesystem::exists("../assets", ec)) {
        std::filesystem::create_directories("../saves", ec);
        return "../saves";
    }
    std::filesystem::create_directories("saves", ec);
    return "saves";
}

std::string WorldSave::sanitizeWorldName(const std::string& name) {
    std::string clean;
    clean.reserve(name.size());
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == ' ') {
            clean += (c == ' ') ? '_' : c;
        }
    }
    if (clean.empty()) clean = "World";
    return clean;
}

std::string WorldSave::getWorldPath(const std::string& worldName) {
    const std::string dir = getSavesDirectory();
    return dir + "/" + sanitizeWorldName(worldName) + ".dat";
}

bool WorldSave::peekWorld(const std::string& path, std::string& outName, uint32_t& outSeed) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    uint32_t magic = 0;
    uint32_t version = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (magic != SAVE_MAGIC || (version != SAVE_VERSION_1 && version != SAVE_VERSION_2)) {
        log::error("Unrecognised save header (magic 0x%08X, version %u) in %s -- "
                   "file may be corrupt or from a newer build; ignoring it.",
                   magic, version, path.c_str());
        return false;
    }

    if (version == SAVE_VERSION_2) {
        uint32_t nameLen = 0;
        file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        if (nameLen > 128) nameLen = 128;
        std::vector<char> buf(nameLen + 1, 0);
        file.read(buf.data(), static_cast<std::streamsize>(nameLen));
        outName = std::string(buf.data());
    } else {
        // Version 1 fallback: extract filename stem
        std::filesystem::path p(path);
        outName = p.stem().string();
    }

    file.read(reinterpret_cast<char*>(&outSeed), sizeof(outSeed));
    return true;
}

std::vector<WorldMetadata> WorldSave::listSavedWorlds() {
    std::vector<WorldMetadata> list;
    std::error_code ec;

    const std::string primaryDir = getSavesDirectory();
    const std::vector<std::string> searchDirs = { primaryDir, "saves", "../../saves", "../saves", "." };

    for (const auto& dir : searchDirs) {
        if (!std::filesystem::exists(dir, ec)) continue;

        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (entry.is_regular_file() && entry.path().extension() == ".dat") {
                const std::string pathStr = entry.path().string();
                const std::string filename = entry.path().filename().string();

                // If save is in a legacy/local build folder, auto-migrate to primary project saves dir
                std::string targetPath = pathStr;
                if (dir != primaryDir && primaryDir != ".") {
                    const std::string newPath = primaryDir + "/" + filename;
                    if (!std::filesystem::exists(newPath, ec)) {
                        std::filesystem::copy_file(pathStr, newPath, std::filesystem::copy_options::overwrite_existing, ec);
                        if (!ec) {
                            log::info("Migrated save file '%s' to persistent directory '%s'", filename.c_str(), primaryDir.c_str());
                            targetPath = newPath;
                            // Remove the legacy copy. Otherwise a world deleted
                            // from the primary directory is resurrected by the
                            // stale file on the next scan.
                            std::error_code removeEc;
                            std::filesystem::remove(pathStr, removeEc);
                        }
                    } else {
                        targetPath = newPath;
                    }
                }

                bool alreadyInList = false;
                for (const auto& w : list) {
                    if (std::filesystem::path(w.path).filename() == filename) {
                        alreadyInList = true;
                        break;
                    }
                }
                if (!alreadyInList) {
                    std::string name;
                    uint32_t seed = 0;
                    if (peekWorld(targetPath, name, seed)) {
                        list.push_back({ name.empty() ? entry.path().stem().string() : name, targetPath, seed });
                    }
                }
            }
        }
    }

    return list;
}

bool WorldSave::saveExists(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return file.good();
}

bool WorldSave::saveGame(const std::string& path, const std::string& worldName, uint32_t seed,
                         const World& world, const Player& player,
                         int selectedSlot, const BlockId* hotbar, const BlockId* inventory) {
    std::error_code ec;
    std::filesystem::path fsPath(path);
    if (fsPath.has_parent_path()) {
        std::filesystem::create_directories(fsPath.parent_path(), ec);
    }

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        log::error("Failed to open save file for writing: %s", path.c_str());
        return false;
    }

    const uint32_t magic = SAVE_MAGIC;
    const uint32_t version = SAVE_VERSION_2;
    const int32_t chunksX = world.chunksX();
    const int32_t chunksZ = world.chunksZ();
    const int32_t chunkW = Chunk::W;
    const int32_t chunkH = Chunk::H;
    const int32_t chunkD = Chunk::D;

    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));

    // World name (Version 2)
    const uint32_t nameLen = static_cast<uint32_t>(worldName.size());
    file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
    if (nameLen > 0) {
        file.write(worldName.data(), static_cast<std::streamsize>(nameLen));
    }

    file.write(reinterpret_cast<const char*>(&seed), sizeof(seed));
    file.write(reinterpret_cast<const char*>(&chunksX), sizeof(chunksX));
    file.write(reinterpret_cast<const char*>(&chunksZ), sizeof(chunksZ));
    file.write(reinterpret_cast<const char*>(&chunkW), sizeof(chunkW));
    file.write(reinterpret_cast<const char*>(&chunkH), sizeof(chunkH));
    file.write(reinterpret_cast<const char*>(&chunkD), sizeof(chunkD));

    // Player Data
    const glm::vec3 pos = player.position();
    const float yaw = player.yaw();
    const float pitch = player.pitch();
    const uint8_t flying = player.isFlying() ? 1 : 0;
    const uint32_t selSlot = static_cast<uint32_t>(selectedSlot);

    file.write(reinterpret_cast<const char*>(&pos), sizeof(pos));
    file.write(reinterpret_cast<const char*>(&yaw), sizeof(yaw));
    file.write(reinterpret_cast<const char*>(&pitch), sizeof(pitch));
    file.write(reinterpret_cast<const char*>(&flying), sizeof(flying));
    file.write(reinterpret_cast<const char*>(&selSlot), sizeof(selSlot));

    for (int i = 0; i < 8; ++i) {
        const uint8_t b = static_cast<uint8_t>(hotbar[i]);
        file.write(reinterpret_cast<const char*>(&b), sizeof(b));
    }
    for (int i = 0; i < 24; ++i) {
        const uint8_t b = static_cast<uint8_t>(inventory[i]);
        file.write(reinterpret_cast<const char*>(&b), sizeof(b));
    }

    // Chunk Voxel Data
    const auto& chunks = world.chunks();
    const uint32_t totalChunks = static_cast<uint32_t>(chunks.size());
    file.write(reinterpret_cast<const char*>(&totalChunks), sizeof(totalChunks));

    for (const auto& chunk : chunks) {
        if (!chunk) continue;
        const int32_t cx = chunk->chunkX();
        const int32_t cz = chunk->chunkZ();
        const std::vector<RLERun> runs = chunk->rleCompress();
        const uint32_t runCount = static_cast<uint32_t>(runs.size());

        file.write(reinterpret_cast<const char*>(&cx), sizeof(cx));
        file.write(reinterpret_cast<const char*>(&cz), sizeof(cz));
        file.write(reinterpret_cast<const char*>(&runCount), sizeof(runCount));

        for (const auto& run : runs) {
            file.write(reinterpret_cast<const char*>(&run.count), sizeof(run.count));
            const uint8_t b = static_cast<uint8_t>(run.block);
            file.write(reinterpret_cast<const char*>(&b), sizeof(b));
        }
    }

    log::info("World '%s' saved successfully to %s", worldName.c_str(), path.c_str());
    return true;
}

bool WorldSave::loadGame(const std::string& path, std::string& outWorldName, uint32_t& outSeed,
                         World& world, Player& player,
                         int& selectedSlot, BlockId* hotbar, BlockId* inventory) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        log::error("Failed to open save file for reading: %s", path.c_str());
        return false;
    }

    uint32_t magic = 0;
    uint32_t version = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (magic != SAVE_MAGIC || (version != SAVE_VERSION_1 && version != SAVE_VERSION_2)) {
        log::error("Invalid save file header or unsupported version in: %s", path.c_str());
        return false;
    }

    if (version == SAVE_VERSION_2) {
        uint32_t nameLen = 0;
        file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        if (nameLen > 128) nameLen = 128;
        std::vector<char> buf(nameLen + 1, 0);
        file.read(buf.data(), static_cast<std::streamsize>(nameLen));
        outWorldName = std::string(buf.data());
    } else {
        std::filesystem::path p(path);
        outWorldName = p.stem().string();
    }

    uint32_t seed = 0;
    int32_t chunksX = 0;
    int32_t chunksZ = 0;
    int32_t chunkW = 0;
    int32_t chunkH = 0;
    int32_t chunkD = 0;

    file.read(reinterpret_cast<char*>(&seed), sizeof(seed));
    file.read(reinterpret_cast<char*>(&chunksX), sizeof(chunksX));
    file.read(reinterpret_cast<char*>(&chunksZ), sizeof(chunksZ));
    file.read(reinterpret_cast<char*>(&chunkW), sizeof(chunkW));
    file.read(reinterpret_cast<char*>(&chunkH), sizeof(chunkH));
    file.read(reinterpret_cast<char*>(&chunkD), sizeof(chunkD));

    outSeed = seed;

    // Player Data
    glm::vec3 pos{0.0f};
    float yaw = -90.0f;
    float pitch = -10.0f;
    uint8_t flying = 0;
    uint32_t selSlot = 0;

    file.read(reinterpret_cast<char*>(&pos), sizeof(pos));
    file.read(reinterpret_cast<char*>(&yaw), sizeof(yaw));
    file.read(reinterpret_cast<char*>(&pitch), sizeof(pitch));
    file.read(reinterpret_cast<char*>(&flying), sizeof(flying));
    file.read(reinterpret_cast<char*>(&selSlot), sizeof(selSlot));

    // Chunk Voxel Data
    if (chunksX <= 0 || chunksZ <= 0) {
        chunksX = config::WORLD_CHUNKS_X;
        chunksZ = config::WORLD_CHUNKS_Z;
    }
    world.init(chunksX, chunksZ, seed);

    // Helper to sanitize block IDs from save files (forward compatibility for unknown/future blocks)
    auto sanitizeBlock = [](uint8_t raw) -> BlockId {
        if (raw >= static_cast<uint8_t>(BlockId::Count)) {
            return BlockId::Stone; // Graceful fallback
        }
        return static_cast<BlockId>(raw);
    };

    for (int i = 0; i < 8; ++i) {
        uint8_t b = 0;
        file.read(reinterpret_cast<char*>(&b), sizeof(b));
        hotbar[i] = sanitizeBlock(b);
    }
    for (int i = 0; i < 24; ++i) {
        uint8_t b = 0;
        file.read(reinterpret_cast<char*>(&b), sizeof(b));
        inventory[i] = sanitizeBlock(b);
    }

    uint32_t totalChunks = 0;
    file.read(reinterpret_cast<char*>(&totalChunks), sizeof(totalChunks));

    for (uint32_t c = 0; c < totalChunks; ++c) {
        int32_t cx = 0;
        int32_t cz = 0;
        uint32_t runCount = 0;

        file.read(reinterpret_cast<char*>(&cx), sizeof(cx));
        file.read(reinterpret_cast<char*>(&cz), sizeof(cz));
        file.read(reinterpret_cast<char*>(&runCount), sizeof(runCount));

        std::vector<RLERun> runs;
        runs.reserve(runCount);

        for (uint32_t r = 0; r < runCount; ++r) {
            uint16_t count = 0;
            uint8_t blockId = 0;
            file.read(reinterpret_cast<char*>(&count), sizeof(count));
            file.read(reinterpret_cast<char*>(&blockId), sizeof(blockId));
            runs.push_back({ count, sanitizeBlock(blockId) });
        }

        if (Chunk* chunk = world.chunkAt(cx, cz)) {
            chunk->decompressRle(runs);
            chunk->dirty = true;
        }
    }

    world.computeWorldLighting();

    // Set player position and orientation after world chunks are allocated
    const float maxW = static_cast<float>(world.widthBlocks());
    const float maxD = static_cast<float>(world.depthBlocks());
    const float maxH = static_cast<float>(world.heightBlocks());
    if (pos.x <= 0.0f || pos.x >= maxW || pos.z <= 0.0f || pos.z >= maxD || pos.y <= 0.0f || pos.y >= maxH) {
        player.spawnAt(world, maxW * 0.5f, maxD * 0.5f);
    } else {
        player.setPosition(pos);
    }
    player.setRotation(yaw, pitch);
    // Creative mode is not persisted, and flight is only available in creative
    // mode, so a saved flying state must not be restored on load.
    (void)flying;
    player.setFlying(false);
    selectedSlot = std::clamp(static_cast<int>(selSlot), 0, 7);

    log::info("World '%s' (seed %u, %dx%d chunks) loaded successfully from %s",
              outWorldName.c_str(), outSeed, chunksX, chunksZ, path.c_str());
    return true;
}

bool WorldSave::deleteWorld(const std::string& path) {
    std::error_code ec;
    bool removed = std::filesystem::remove(path, ec);

    std::filesystem::path fsPath(path);
    const std::string filename = fsPath.filename().string();
    const std::string primaryDir = getSavesDirectory();
    const std::vector<std::string> searchDirs = { primaryDir, "saves", "../../saves", "../saves", "." };

    for (const auto& dir : searchDirs) {
        const std::string altPath = dir + "/" + filename;
        if (std::filesystem::exists(altPath, ec)) {
            if (std::filesystem::remove(altPath, ec)) {
                removed = true;
            }
        }
    }

    if (removed) {
        log::info("Deleted world save file: %s", path.c_str());
    } else {
        log::error("Failed to delete world save file: %s (%s)", path.c_str(), ec.message().c_str());
    }
    return removed;
}

} // namespace vox

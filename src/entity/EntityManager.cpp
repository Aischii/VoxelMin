#include "entity/EntityManager.hpp"
#include "world/Block.hpp"
#include "world/World.hpp"

#include <algorithm>

namespace vox {
namespace {

uint32_t hashSpawn(uint32_t a, uint32_t b, uint32_t c) {
    uint32_t h = a * 374761393U + b * 668265263U + c * 362437U;
    h ^= h >> 13;
    h *= 0x5bd1e995U;
    h ^= h >> 15;
    return h;
}

} // namespace

void EntityManager::spawnDefaults(const World& world, uint32_t seed) {
    clear();

    const int worldW = world.widthBlocks();
    const int worldD = world.depthBlocks();

    // 1. Spawn Wild Passive Mobs (Pigs & Cows) across the world
    const int targetWildMobs = (worldW <= 180) ? 20 : 72;
    int spawnedWild = 0;

    for (uint32_t i = 0; i < 3000 && spawnedWild < targetWildMobs; ++i) {
        const uint32_t rx = hashSpawn(seed, i, 101);
        const uint32_t rz = hashSpawn(seed, i, 202);
        const uint32_t ryaw = hashSpawn(seed, i, 303);

        const int x = 12 + static_cast<int>(rx % (worldW - 24));
        const int z = 12 + static_cast<int>(rz % (worldD - 24));

        // Find true solid ground surface (probe down past leaves, logs, tall grass)
        int groundY = world.surfaceHeight(x, z);
        while (groundY > 0 && (world.getBlock(x, groundY, z) == BlockId::Leaves ||
                               world.getBlock(x, groundY, z) == BlockId::Wood ||
                               world.getBlock(x, groundY, z) == BlockId::WoodX ||
                               world.getBlock(x, groundY, z) == BlockId::WoodZ ||
                               !isSolid(world.getBlock(x, groundY, z)))) {
            groundY--;
        }

        if (groundY < 26) continue; // Skip underwater/sea level areas

        const BlockId surfaceBlock = world.getBlock(x, groundY, z);
        if (surfaceBlock != BlockId::Grass && surfaceBlock != BlockId::Dirt) continue;

        // Ensure clearance for mob body above ground
        const BlockId above1 = world.getBlock(x, groundY + 1, z);
        const BlockId above2 = world.getBlock(x, groundY + 2, z);
        if (isSolid(above1) || isSolid(above2) || isLiquid(above1)) continue;

        const MobType type = (spawnedWild % 2 == 0) ? MobType::Pig : MobType::Cow;
        const float yaw = static_cast<float>(ryaw % 360);

        m_mobs.push_back(std::make_unique<Mob>(type, glm::vec3(static_cast<float>(x) + 0.5f,
                                                               static_cast<float>(groundY + 1) + 0.001f,
                                                               static_cast<float>(z) + 0.5f),
                                               yaw));
        spawnedWild++;
    }

    // 2. Spawn Pigman Villagers in each generated Village
    for (size_t vi = 0; vi < world.villages().size(); ++vi) {
        const auto& village = world.villages()[vi];
        const int pigmenPerVillage = 5;

        for (int p = 0; p < pigmenPerVillage; ++p) {
            const uint32_t px = hashSpawn(seed + static_cast<uint32_t>(vi) * 100, static_cast<uint32_t>(p), 404);
            const uint32_t pz = hashSpawn(seed + static_cast<uint32_t>(vi) * 100, static_cast<uint32_t>(p), 505);
            const uint32_t pyaw = hashSpawn(seed + static_cast<uint32_t>(vi) * 100, static_cast<uint32_t>(p), 606);

            const int offsetX = static_cast<int>(px % 16) - 8;
            const int offsetZ = static_cast<int>(pz % 16) - 8;

            const int spawnX = std::clamp(static_cast<int>(village.center.x) + offsetX, 4, worldW - 4);
            const int spawnZ = std::clamp(static_cast<int>(village.center.z) + offsetZ, 4, worldD - 4);
            int spawnY = world.surfaceHeight(spawnX, spawnZ);
            while (spawnY > 0 && !isSolid(world.getBlock(spawnX, spawnY, spawnZ))) {
                spawnY--;
            }

            if (spawnY < 26) continue;

            auto pigman = std::make_unique<Mob>(MobType::PigmanVillager,
                                                glm::vec3(static_cast<float>(spawnX) + 0.5f,
                                                          static_cast<float>(spawnY + 1) + 0.001f,
                                                          static_cast<float>(spawnZ) + 0.5f),
                                                static_cast<float>(pyaw % 360));
            pigman->setHomeVillage(village.center);
            m_mobs.push_back(std::move(pigman));
        }
    }
}

void EntityManager::alertNearbyPigmen(const glm::vec3& position, float radius) {
    for (auto& mob : m_mobs) {
        if (!mob || !mob->isAlive()) continue;
        if (mob->type() == MobType::PigmanVillager) {
            const float dist = glm::distance(mob->position(), position);
            if (dist <= radius) {
                mob->alertAggro(position);
            }
        }
    }
}

void EntityManager::addMob(std::unique_ptr<Mob> mob) {
    if (mob) {
        m_mobs.push_back(std::move(mob));
    }
}

void EntityManager::clear() {
    m_mobs.clear();
}

void EntityManager::update(float dt, const World& world, const glm::vec3& playerPos) {
    for (auto it = m_mobs.begin(); it != m_mobs.end(); ) {
        if (!(*it)->isAlive()) {
            it = m_mobs.erase(it);
        } else {
            (*it)->update(dt, world, playerPos);
            ++it;
        }
    }
}

Mob* EntityManager::hitTest(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDist) {
    Mob* closestMob = nullptr;
    float closestDist = maxDist + 1.0f;

    for (auto& mob : m_mobs) {
        if (!mob || !mob->isAlive()) continue;

        float dist = 0.0f;
        if (mob->collidesWithRay(rayOrigin, rayDir, maxDist, dist)) {
            if (dist < closestDist) {
                closestDist = dist;
                closestMob = mob.get();
            }
        }
    }

    return closestMob;
}

void EntityManager::buildMesh(std::vector<Vertex>& outVertices, const World& world) const {
    for (const auto& mob : m_mobs) {
        if (!mob || !mob->isAlive()) continue;
        mob->appendGeometry(outVertices, world);
    }
}

} // namespace vox

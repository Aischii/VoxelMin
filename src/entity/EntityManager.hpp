#pragma once
#include "entity/ItemEntity.hpp"
#include "entity/Mob.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace vox {

class World;

class EntityManager {
public:
    EntityManager() = default;

    void spawnDefaults(const World& world, uint32_t seed);
    void addMob(std::unique_ptr<Mob> mob);
    void spawnItem(BlockId id, const glm::vec3& position, int count = 1, const glm::vec3& initialVelocity = glm::vec3(0.0f));
    void clear();

    void update(float dt, const World& world, const glm::vec3& playerPos,
                const std::function<bool(BlockId, int)>& onPickup = nullptr);
    Mob* hitTest(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDist);
    void alertNearbyPigmen(const glm::vec3& position, float radius = 14.0f);
    void buildMesh(std::vector<Vertex>& outVertices, const World& world) const;

    const std::vector<std::unique_ptr<Mob>>& mobs() const { return m_mobs; }
    std::vector<std::unique_ptr<Mob>>& mobs() { return m_mobs; }

    const std::vector<std::unique_ptr<ItemEntity>>& items() const { return m_items; }
    std::vector<std::unique_ptr<ItemEntity>>& items() { return m_items; }

private:
    std::vector<std::unique_ptr<Mob>> m_mobs;
    std::vector<std::unique_ptr<ItemEntity>> m_items;
};

} // namespace vox


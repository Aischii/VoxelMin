#pragma once
#include "entity/Mob.hpp"
#include <memory>
#include <vector>

namespace vox {

class World;

class EntityManager {
public:
    EntityManager() = default;

    void spawnDefaults(const World& world, uint32_t seed);
    void addMob(std::unique_ptr<Mob> mob);
    void clear();

    void update(float dt, const World& world, const glm::vec3& playerPos);
    Mob* hitTest(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDist);
    void alertNearbyPigmen(const glm::vec3& position, float radius = 14.0f);
    void buildMesh(std::vector<Vertex>& outVertices, const World& world) const;

    const std::vector<std::unique_ptr<Mob>>& mobs() const { return m_mobs; }
    std::vector<std::unique_ptr<Mob>>& mobs() { return m_mobs; }

private:
    std::vector<std::unique_ptr<Mob>> m_mobs;
};

} // namespace vox

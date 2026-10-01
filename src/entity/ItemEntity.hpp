#pragma once
#include "world/Block.hpp"
#include "render/Mesh.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace vox {

class World;

// Represents a floating, collectible item entity in the voxel world.
// Drops from broken blocks, experiences gravity, bounces on terrain,
// is magnetically attracted to the player, and can be collected.
class ItemEntity {
public:
    ItemEntity(BlockId id, const glm::vec3& position, int count = 1, const glm::vec3& initialVelocity = glm::vec3(0.0f));

    void update(float dt, const World& world, const glm::vec3& playerPos);
    void appendGeometry(std::vector<Vertex>& vertices, const World& world) const;

    BlockId blockId() const { return m_blockId; }
    int count() const { return m_count; }
    void setCount(int count) { m_count = count; }

    const glm::vec3& position() const { return m_position; }
    const glm::vec3& velocity() const { return m_velocity; }

    bool isAlive() const { return m_age < 300.0f; } // 5 minutes despawn timer
    bool canPickup() const { return m_pickupDelay <= 0.0f; }
    bool isWithinPickupRange(const glm::vec3& playerPos) const;

private:
    bool collides(const World& world, const glm::vec3& pos) const;
    void moveAxis(const World& world, float delta, int axis);

    BlockId m_blockId = BlockId::Air;
    int m_count = 1;
    glm::vec3 m_position{0.0f};
    glm::vec3 m_velocity{0.0f};

    float m_age = 0.0f;
    float m_pickupDelay = 0.6f;
    bool m_onGround = false;

    static constexpr float HALF_SIZE = 0.12f;
    static constexpr float GRAVITY   = 20.0f;
    static constexpr float MAGNET_RADIUS = 1.6f;
    static constexpr float PICKUP_RADIUS = 0.95f;
};

} // namespace vox

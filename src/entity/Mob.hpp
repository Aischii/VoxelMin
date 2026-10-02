#pragma once
#include "entity/MobAi.hpp"
#include "render/Mesh.hpp"
#include "world/Block.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace vox {

class World;

enum class MobState : uint8_t {
    Idle,
    Wander,
    Panic,
    Hostile,
    ReturnToVillage,
    StalkFrozen,
    ShadowSprint,
    SeekLight,
    RetreatShadows,
};

class Mob {
public:
    Mob(MobType type, const glm::vec3& position, float yaw = 0.0f);

    MobType type() const { return m_type; }
    const glm::vec3& position() const { return m_position; }
    void setPosition(const glm::vec3& pos) { m_position = pos; }

    float yaw() const { return m_yaw; }
    void setYaw(float yaw) { m_yaw = yaw; }

    int health() const { return m_health; }
    bool isAlive() const { return m_health > 0; }
    float hurtTimer() const { return m_hurtTimer; }
    bool onGround() const { return m_onGround; }
    int stepUpCount() const { return m_stepUps; }
    int jumpCount() const { return m_jumps; }
    bool isHostile() const { return m_state == MobState::Hostile || m_state == MobState::ShadowSprint; }
    bool isFrozen() const { return m_state == MobState::StalkFrozen; }
    bool canAttack() const { return m_attackCooldown <= 0.0f; }
    void resetAttackCooldown(float cd = 1.0f) { m_attackCooldown = cd; }

    void setHomeVillage(const glm::vec3& center) { m_homeVillage = center; m_hasHome = true; }
    const glm::vec3& homeVillage() const { return m_homeVillage; }
    bool hasHome() const { return m_hasHome; }

    void alertAggro(const glm::vec3& targetPos);
    void takeDamage(int amount, const glm::vec3& sourcePos);
    void update(float dt, World& world, const glm::vec3& playerPos, const glm::vec3& playerCamFront = glm::vec3(0.0f, 0.0f, -1.0f));

    bool collidesWithRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDist, float& outDist) const;
    void appendGeometry(std::vector<Vertex>& vertices, const World& world) const;

    float halfWidth() const;
    float halfHeight() const;

private:
    void updateAI(float dt, World& world, const glm::vec3& playerPos, const glm::vec3& playerCamFront);
    void updatePhysics(float dt, const World& world);
    void updateStuckResponse(float dt);

    bool collides(const World& world, const glm::vec3& feet) const;
    void moveAxis(const World& world, float delta, int axis);
    bool tryStepUp(const World& world, const glm::vec3& candidate);
    int dropAhead(const World& world, float distance) const;
    bool tryJump();
    void updateForwardVector();
    void rerollHeading();

    MobType m_type = MobType::Pig;
    glm::vec3 m_position{0.0f};
    glm::vec3 m_velocity{0.0f};
    float m_yaw = 0.0f;
    float m_targetYaw = 0.0f;
    float m_headYaw = 0.0f;
    float m_headPitch = 0.0f;

    // Cached sin/cos of m_yaw
    float m_cachedYaw = 1000.0f;
    float m_fwdX = 0.0f;
    float m_fwdZ = 1.0f;

    bool m_onGround = false;
    bool m_inWater = false;
    bool m_blockedThisFrame = false;
    int m_health = 10;
    float m_hurtTimer = 0.0f;

    float m_blockedTimer = 0.0f;
    float m_jumpCooldown = 0.0f;
    float m_attackCooldown = 0.0f;
    float m_knockbackTimer = 0.0f;
    float m_aggroTimer = 0.0f;
    int m_stepUps = 0;
    int m_jumps = 0;

    MobState m_state = MobState::Idle;
    float m_stateTimer = 2.0f;
    float m_animTime = 0.0f;
    float m_moveSpeed = 0.0f;
    glm::vec3 m_panicSource{0.0f};

    glm::vec3 m_homeVillage{0.0f};
    bool m_hasHome = false;

    AiGoalMask m_goals = 0;
};

} // namespace vox

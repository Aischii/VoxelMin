#pragma once
#include "render/Camera.hpp"
#include "render/Mesh.hpp"

#include <algorithm>
#include <glm/glm.hpp>
#include <vector>

namespace vox {

class World;
class Input;
class AudioEngine;

enum class Perspective {
    FirstPerson,
    ThirdPersonBack,
    ThirdPersonFront,
};

// Player state and physics. Owns the camera and applies mouse-look plus
// axis-by-axis AABB collision against the voxel world.
//
// "position" is the centre of the player's feet. The collision box is
// 0.6 x 1.8 x 0.6 (half extents 0.3 / 0.9 / 0.3).
class Player {
public:
    void spawnAt(const World& world, float x, float z);
    void update(float dt, const Input& input, const World& world, AudioEngine* audio = nullptr);

    Camera& camera() { return m_camera; }
    const Camera& camera() const { return m_camera; }

    const glm::vec3& position() const { return m_position; }
    void setPosition(const glm::vec3& pos);
    glm::vec3 eyePosition() const;

    float yaw() const { return m_yaw; }
    float pitch() const { return m_pitch; }
    float roll() const { return m_roll; }
    void setRotation(float yaw, float pitch, float roll = 0.0f);

    bool isFlying() const { return m_flying; }
    void setFlying(bool flying) { m_flying = flying; }
    void toggleFlying() { m_flying = !m_flying; }

    bool isSprinting() const { return m_sprinting; }
    void setSprinting(bool sprinting) { m_sprinting = sprinting; }

    bool isInWater() const { return m_inWater; }

    void setMouseSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }
    float mouseSensitivity() const { return m_mouseSensitivity; }

    Perspective perspective() const { return m_perspective; }
    void setPerspective(Perspective p) { m_perspective = p; }
    void cyclePerspective();

    void triggerSwing() { m_swingTimer = 0.25f; }
    float swingProgress() const { return std::clamp(1.0f - (m_swingTimer / 0.25f), 0.0f, 1.0f); }
    float animTime() const { return m_animTime; }

    void appendGeometry(std::vector<Vertex>& vertices, const World& world) const;
    void appendFirstPersonArm(std::vector<Vertex>& vertices, const World& world) const;

private:
    void applyLook(const Input& input);
    bool collides(const World& world, const glm::vec3& feetPosition) const;
    void moveAxis(const World& world, float delta, int axis);

    glm::vec3 m_position{0.0f};
    glm::vec3 m_velocity{0.0f};
    float m_yaw = -90.0f;
    float m_pitch = -10.0f;
    float m_roll = 0.0f;
    bool m_onGround = false;
    bool m_flying = false;
    bool m_sprinting = false;
    bool m_inWater = false;
    float m_timeSinceWPress = 100.0f;
    float m_mouseSensitivity = 0.12f;

    // View bobbing & dynamics
    float m_bobTimer = 0.0f;
    float m_bobIntensity = 0.0f;
    float m_stepDistance = 0.0f;

    // Jump queueing & air control
    float m_jumpQueueTimer = 0.0f;

    // Perspective & animations
    Perspective m_perspective = Perspective::FirstPerson;
    float m_swingTimer = 0.0f;
    float m_animTime = 0.0f;

    Camera m_camera;

    static constexpr float HALF_WIDTH           = 0.3f;
    static constexpr float HALF_HEIGHT          = 0.9f;

    static constexpr float WALK_SPEED           = 4.8f;
    static constexpr float SPRINT_SPEED         = 7.0f;
    static constexpr float FLY_SPEED            = 14.0f;
    static constexpr float JUMP_SPEED           = 8.5f;
    static constexpr float GRAVITY              = 26.0f;

    // Responsive voxel movement & air control parameters
    static constexpr float GROUND_ACCEL_RATE    = 18.0f;
    static constexpr float GROUND_DECEL_RATE    = 20.0f;
    static constexpr float AIR_ACCEL_RATE       = 8.5f;
    static constexpr float AIR_DECEL_RATE       = 2.0f;
    static constexpr float JUMP_QUEUE_DURATION  = 0.15f;
};

} // namespace vox

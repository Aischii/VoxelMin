#include "player/Player.hpp"
#include "audio/AudioEngine.hpp"
#include "core/Config.hpp"
#include "input/Input.hpp"
#include "world/Block.hpp"
#include "world/World.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace vox {

void Player::spawnAt(const World& world, float x, float z) {
    const int wx = static_cast<int>(x);
    const int wz = static_cast<int>(z);
    const int surface = world.surfaceHeight(wx, wz);
    m_position = glm::vec3(x + 0.5f, static_cast<float>(surface + 1) + 0.001f, z + 0.5f);
    m_velocity = glm::vec3(0.0f);
    m_onGround = true;
}

void Player::setPosition(const glm::vec3& pos) {
    m_position = pos;
    m_velocity = glm::vec3(0.0f);
    m_camera.setPosition(eyePosition());
}

void Player::setRotation(float yaw, float pitch, float roll) {
    m_yaw = yaw;
    m_pitch = std::clamp(pitch, -89.0f, 89.0f);
    m_roll = roll;
    m_camera.setRotation(m_yaw, m_pitch, m_roll);
}

glm::vec3 Player::eyePosition() const {
    return m_position + glm::vec3(0.0f, config::PLAYER_EYE_HEIGHT, 0.0f);
}

void Player::applyLook(const Input& input) {
    if (!input.cursorCaptured()) return;

    m_yaw += input.mouseDeltaX() * m_mouseSensitivity;
    m_pitch -= input.mouseDeltaY() * m_mouseSensitivity;
    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
}

bool Player::collides(const World& world, const glm::vec3& feet) const {
    const glm::vec3 minP(feet.x - HALF_WIDTH, feet.y, feet.z - HALF_WIDTH);
    const glm::vec3 maxP(feet.x + HALF_WIDTH, feet.y + 2.0f * HALF_HEIGHT, feet.z + HALF_WIDTH);

    const float eps = 1e-4f;
    const int x0 = static_cast<int>(std::floor(minP.x));
    const int x1 = static_cast<int>(std::floor(maxP.x - eps));
    const int y0 = static_cast<int>(std::floor(minP.y));
    const int y1 = static_cast<int>(std::floor(maxP.y - eps));
    const int z0 = static_cast<int>(std::floor(minP.z));
    const int z1 = static_cast<int>(std::floor(maxP.z - eps));

    for (int y = y0; y <= y1; ++y) {
        for (int z = z0; z <= z1; ++z) {
            for (int x = x0; x <= x1; ++x) {
                if (isSolid(world.getBlock(x, y, z))) return true;
            }
        }
    }
    return false;
}

void Player::moveAxis(const World& world, float delta, int axis) {
    if (delta == 0.0f) return;

    glm::vec3 candidate = m_position;
    candidate[axis] += delta;

    if (collides(world, candidate)) {
        if (axis == 1 && delta < 0.0f) m_onGround = true;
        m_velocity[axis] = 0.0f;
        return; // keep previous position on that axis (simple, stable)
    }
    m_position = candidate;
}

void Player::update(float dt, const Input& input, const World& world, AudioEngine* audio) {
    applyLook(input);

    m_timeSinceWPress += dt;

    if (input.cursorCaptured()) {
        if (input.keyPressed(GLFW_KEY_W)) {
            if (m_timeSinceWPress < 0.28f) {
                m_sprinting = true;
            }
            m_timeSinceWPress = 0.0f;
        }

        if (input.keyDown(GLFW_KEY_LEFT_CONTROL) && input.keyDown(GLFW_KEY_W)) {
            m_sprinting = true;
        }
    }

    if (!input.cursorCaptured() || !input.keyDown(GLFW_KEY_W)) {
        m_sprinting = false;
    }

    glm::vec3 forward = m_camera.front();
    forward.y = 0.0f;
    if (glm::length(forward) < 1e-5f) forward = glm::vec3(0.0f, 0.0f, -1.0f);
    forward = glm::normalize(forward);

    glm::vec3 right = m_camera.right();
    right.y = 0.0f;
    if (glm::length(right) < 1e-5f) right = glm::vec3(1.0f, 0.0f, 0.0f);
    right = glm::normalize(right);

    glm::vec3 wish(0.0f);
    if (input.cursorCaptured()) {
        if (input.keyDown(GLFW_KEY_W)) wish += forward;
        if (input.keyDown(GLFW_KEY_S)) wish -= forward;
        if (input.keyDown(GLFW_KEY_D)) wish += right;
        if (input.keyDown(GLFW_KEY_A)) wish -= right;
        if (glm::length(wish) > 1e-5f) wish = glm::normalize(wish);
    }

    // Check if player is submerged in water
    const int bx = static_cast<int>(std::floor(m_position.x));
    const int byFeet = static_cast<int>(std::floor(m_position.y));
    const int byMid = static_cast<int>(std::floor(m_position.y + 0.9f));
    const int bz = static_cast<int>(std::floor(m_position.z));
    m_inWater = (isLiquid(world.getBlock(bx, byFeet, bz)) || isLiquid(world.getBlock(bx, byMid, bz)));

    // Jump queueing & auto-bunnyhop
    if (input.cursorCaptured()) {
        if (input.keyPressed(GLFW_KEY_SPACE)) {
            m_jumpQueueTimer = JUMP_QUEUE_DURATION;
        } else if (input.keyDown(GLFW_KEY_SPACE)) {
            m_jumpQueueTimer = std::max(m_jumpQueueTimer, dt * 2.0f);
        }
    }
    m_jumpQueueTimer = std::max(0.0f, m_jumpQueueTimer - dt);
    const bool wantsJump = (m_jumpQueueTimer > 0.0f);

    if (m_flying) {
        const float speed = FLY_SPEED * ((input.cursorCaptured() && (input.keyDown(GLFW_KEY_LEFT_CONTROL) || m_sprinting)) ? 2.5f : 1.0f);
        m_velocity = wish * speed;
        if (input.cursorCaptured()) {
            if (input.keyDown(GLFW_KEY_SPACE))      m_velocity.y = speed;
            if (input.keyDown(GLFW_KEY_LEFT_SHIFT)) m_velocity.y = -speed;
        }
    } else if (m_inWater) {
        // Water swimming & buoyancy physics
        const float waterSpeed = WALK_SPEED * 0.70f;
        m_velocity.x = wish.x * waterSpeed;
        m_velocity.z = wish.z * waterSpeed;

        if (input.cursorCaptured() && input.keyDown(GLFW_KEY_SPACE)) {
            m_velocity.y = 4.2f; // Active upward swimming
        } else if (input.cursorCaptured() && input.keyDown(GLFW_KEY_LEFT_SHIFT)) {
            m_velocity.y = -4.0f; // Dive down
        } else {
            // Gentle sinking drift with fluid drag
            m_velocity.y -= 7.0f * dt;
            if (m_velocity.y < -3.5f) m_velocity.y = -3.5f;
        }
    } else if (m_onGround) {
        // Ground physics: responsive ground acceleration & friction
        const float wishSpeed = m_sprinting ? SPRINT_SPEED : WALK_SPEED;
        const glm::vec3 targetVel = (glm::length(wish) > 1e-4f) ? (wish * wishSpeed) : glm::vec3(0.0f);

        if (wantsJump) {
            // Jump immediately, launching with current/desired velocity in any direction
            if (glm::length(wish) > 1e-4f) {
                m_velocity.x = targetVel.x;
                m_velocity.z = targetVel.z;
            }
            m_velocity.y = JUMP_SPEED;
            m_onGround = false;
            m_jumpQueueTimer = 0.0f;
        } else {
            if (glm::length(wish) > 1e-4f) {
                // Snappy ground acceleration towards wish direction (forward, backward, sideways, diagonals)
                m_velocity.x = glm::mix(m_velocity.x, targetVel.x, std::min(1.0f, dt * GROUND_ACCEL_RATE));
                m_velocity.z = glm::mix(m_velocity.z, targetVel.z, std::min(1.0f, dt * GROUND_ACCEL_RATE));
            } else {
                // Crisp ground deceleration when keys are released
                m_velocity.x = glm::mix(m_velocity.x, 0.0f, std::min(1.0f, dt * GROUND_DECEL_RATE));
                m_velocity.z = glm::mix(m_velocity.z, 0.0f, std::min(1.0f, dt * GROUND_DECEL_RATE));
                if (std::abs(m_velocity.x) < 0.01f) m_velocity.x = 0.0f;
                if (std::abs(m_velocity.z) < 0.01f) m_velocity.z = 0.0f;
            }
        }
    } else {
        // Air physics: full responsive air strafe & air steering (Minecraft-style)
        if (glm::length(wish) > 1e-4f) {
            const float airWishSpeed = m_sprinting ? SPRINT_SPEED : WALK_SPEED;
            const glm::vec3 targetAirVel = wish * airWishSpeed;
            // Responsive mid-air steering for strafing left/right and jumping around corners
            m_velocity.x = glm::mix(m_velocity.x, targetAirVel.x, std::min(1.0f, dt * AIR_ACCEL_RATE));
            m_velocity.z = glm::mix(m_velocity.z, targetAirVel.z, std::min(1.0f, dt * AIR_ACCEL_RATE));
        } else {
            // Gentle air resistance
            m_velocity.x = glm::mix(m_velocity.x, 0.0f, std::min(1.0f, dt * AIR_DECEL_RATE));
            m_velocity.z = glm::mix(m_velocity.z, 0.0f, std::min(1.0f, dt * AIR_DECEL_RATE));
        }

        // Gravity
        m_velocity.y -= GRAVITY * dt;
        if (m_velocity.y < -50.0f) m_velocity.y = -50.0f;
    }

    m_onGround = false;
    moveAxis(world, m_velocity.x * dt, 0);
    moveAxis(world, m_velocity.y * dt, 1);
    moveAxis(world, m_velocity.z * dt, 2);

    const float hSpeed = std::hypot(m_velocity.x, m_velocity.z);

    if (m_swingTimer > 0.0f) {
        m_swingTimer -= dt;
        if (m_swingTimer < 0.0f) m_swingTimer = 0.0f;
    }

    if (hSpeed > 0.05f) {
        m_animTime += dt * hSpeed * 3.5f;
    }

    // Dynamic FOV adjustment when sprinting / flying fast
    float targetFov = 70.0f;
    if (m_sprinting && glm::length(wish) > 0.1f) {
        targetFov = 78.0f;
    } else if (m_flying && input.cursorCaptured() && input.keyDown(GLFW_KEY_LEFT_CONTROL)) {
        targetFov = 85.0f;
    }
    m_camera.setFov(glm::mix(m_camera.fov(), targetFov, std::min(1.0f, dt * 10.0f)));

    // View bobbing (Minecraft style: smooth positional sway without horizon pitch/roll rocking)
    float targetBobIntensity = 0.0f;
    if (m_onGround && !m_flying && !m_inWater && hSpeed > 0.2f) {
        targetBobIntensity = m_sprinting ? 1.3f : 1.0f;
        const float freqMult = m_sprinting ? 1.25f : 1.0f;
        m_bobTimer += hSpeed * dt * 2.8f * freqMult;

        m_stepDistance += hSpeed * dt;
        const float stepInterval = m_sprinting ? 1.75f : 2.15f;
        if (m_stepDistance >= stepInterval) {
            m_stepDistance = 0.0f;
            if (audio) {
                const int bx = static_cast<int>(std::floor(m_position.x));
                const int by = static_cast<int>(std::floor(m_position.y - 0.2f));
                const int bz = static_cast<int>(std::floor(m_position.z));
                const BlockId blockBelow = world.getBlock(bx, by, bz);
                SoundId stepSnd = SoundId::StepGrass;
                if (blockBelow == BlockId::Wood || blockBelow == BlockId::WoodX ||
                    blockBelow == BlockId::WoodZ || blockBelow == BlockId::Planks ||
                    blockBelow == BlockId::CraftingTable) {
                    stepSnd = SoundId::StepWood;
                } else if (blockBelow == BlockId::Stone || blockBelow == BlockId::Cobblestone ||
                           blockBelow == BlockId::CoalOre || blockBelow == BlockId::IronOre ||
                           blockBelow == BlockId::GoldOre || blockBelow == BlockId::DiamondOre ||
                           blockBelow == BlockId::Bedrock) {
                    stepSnd = SoundId::StepStone;
                } else {
                    stepSnd = SoundId::StepGrass;
                }
                audio->play(stepSnd, m_sprinting ? 0.70f : 0.50f);
            }
        }
    } else {
        targetBobIntensity = 0.0f;
        m_stepDistance = std::min(m_stepDistance, 0.7f);
    }
    m_bobIntensity = glm::mix(m_bobIntensity, targetBobIntensity, std::min(1.0f, dt * 6.0f));

    float bobX = 0.0f;
    float bobY = 0.0f;

    if (m_bobIntensity > 0.001f) {
        // Continuous smooth sinusoidal motion: horizontal sway (left-right) and gentle vertical dip
        bobX = std::sin(m_bobTimer * 0.5f) * 0.022f * m_bobIntensity;
        bobY = -(std::cos(m_bobTimer) * 0.5f + 0.5f) * 0.026f * m_bobIntensity;
    }

    // Keep camera roll and pitch completely level and stable (prevents dizziness/motion sickness)
    m_roll = 0.0f;

    const glm::vec3 eyePos = eyePosition();

    if (m_perspective == Perspective::FirstPerson) {
        const glm::vec3 cameraPos = eyePos + right * bobX + glm::vec3(0.0f, bobY, 0.0f);
        m_camera.setPosition(cameraPos);
        m_camera.setRotation(m_yaw, m_pitch, m_roll);
    } else if (m_perspective == Perspective::ThirdPersonBack) {
        m_camera.setRotation(m_yaw, m_pitch, 0.0f);
        const glm::vec3 lookDir = m_camera.front();
        const float targetDist = 4.0f;
        float actualDist = targetDist;
        for (float d = 0.2f; d <= targetDist; d += 0.1f) {
            const glm::vec3 testPos = eyePos - lookDir * d;
            const int tx = static_cast<int>(std::floor(testPos.x));
            const int ty = static_cast<int>(std::floor(testPos.y));
            const int tz = static_cast<int>(std::floor(testPos.z));
            if (isSolid(world.getBlock(tx, ty, tz))) {
                actualDist = std::max(0.4f, d - 0.25f);
                break;
            }
        }
        m_camera.setPosition(eyePos - lookDir * actualDist);
    } else { // ThirdPersonFront
        m_camera.setRotation(m_yaw, m_pitch, 0.0f);
        const glm::vec3 lookDir = m_camera.front();
        const float targetDist = 4.0f;
        float actualDist = targetDist;
        for (float d = 0.2f; d <= targetDist; d += 0.1f) {
            const glm::vec3 testPos = eyePos + lookDir * d;
            const int tx = static_cast<int>(std::floor(testPos.x));
            const int ty = static_cast<int>(std::floor(testPos.y));
            const int tz = static_cast<int>(std::floor(testPos.z));
            if (isSolid(world.getBlock(tx, ty, tz))) {
                actualDist = std::max(0.4f, d - 0.25f);
                break;
            }
        }
        m_camera.setPosition(eyePos + lookDir * actualDist);
        m_camera.setRotation(m_yaw + 180.0f, -m_pitch, 0.0f);
    }
}

void Player::cyclePerspective() {
    if (m_perspective == Perspective::FirstPerson) {
        m_perspective = Perspective::ThirdPersonBack;
    } else if (m_perspective == Perspective::ThirdPersonBack) {
        m_perspective = Perspective::ThirdPersonFront;
    } else {
        m_perspective = Perspective::FirstPerson;
    }
}

namespace {

glm::vec2 tileMinUV(TextureTile tile) {
    const int idx = static_cast<int>(tile);
    const float invAtlas = 1.0f / static_cast<float>(config::ATLAS_TILES);
    const int col = idx % config::ATLAS_TILES;
    const int row = idx / config::ATLAS_TILES;
    const float tx = static_cast<float>(col) * invAtlas;
    const float ty = static_cast<float>(row) * invAtlas;
    return {tx, ty};
}

constexpr glm::vec2 tileSizeUV() {
    return {1.0f / static_cast<float>(config::ATLAS_TILES), 1.0f / static_cast<float>(config::ATLAS_TILES)};
}

void addOrientedBox(std::vector<Vertex>& vertices,
                    const glm::vec3& center,
                    const glm::vec3& size,
                    const glm::mat4& transform,
                    TextureTile tileTop,
                    TextureTile tileSide,
                    TextureTile tileBottom,
                    TextureTile tileFront,
                    float light,
                    float ao) {
    const glm::vec3 half = size * 0.5f;
    const glm::vec2 tSize = tileSizeUV();

    struct FaceDef {
        glm::vec3 normal;
        glm::vec3 c1, c2, c3, c4;
        TextureTile tile;
    };

    const glm::vec3 p0(-half.x, -half.y, -half.z);
    const glm::vec3 p1( half.x, -half.y, -half.z);
    const glm::vec3 p2( half.x,  half.y, -half.z);
    const glm::vec3 p3(-half.x,  half.y, -half.z);
    const glm::vec3 p4(-half.x, -half.y,  half.z);
    const glm::vec3 p5( half.x, -half.y,  half.z);
    const glm::vec3 p6( half.x,  half.y,  half.z);
    const glm::vec3 p7(-half.x,  half.y,  half.z);

    const FaceDef faces[6] = {
        // Top (+Y)
        { {0, 1, 0}, p7, p6, p2, p3, tileTop },
        // Bottom (-Y)
        { {0, -1, 0}, p4, p0, p1, p5, tileBottom },
        // North / Back (-Z)
        { {0, 0, -1}, p1, p0, p3, p2, tileSide },
        // South / Front (+Z)
        { {0, 0, 1}, p4, p5, p6, p7, tileFront },
        // West / Left (-X)
        { {-1, 0, 0}, p0, p4, p7, p3, tileSide },
        // East / Right (+X)
        { {1, 0, 0}, p5, p1, p2, p6, tileSide },
    };

    for (const auto& f : faces) {
        const glm::vec2 tMin = tileMinUV(f.tile);

        const glm::vec3 v0 = glm::vec3(transform * glm::vec4(center + f.c1, 1.0f));
        const glm::vec3 v1 = glm::vec3(transform * glm::vec4(center + f.c2, 1.0f));
        const glm::vec3 v2 = glm::vec3(transform * glm::vec4(center + f.c3, 1.0f));
        const glm::vec3 v3 = glm::vec3(transform * glm::vec4(center + f.c4, 1.0f));
        const glm::vec3 norm = glm::normalize(glm::mat3(transform) * f.normal);

        vertices.push_back({ v0, norm, {0.0f, 0.0f}, tMin, tSize, ao, light });
        vertices.push_back({ v1, norm, {1.0f, 0.0f}, tMin, tSize, ao, light });
        vertices.push_back({ v2, norm, {1.0f, 1.0f}, tMin, tSize, ao, light });

        vertices.push_back({ v0, norm, {0.0f, 0.0f}, tMin, tSize, ao, light });
        vertices.push_back({ v2, norm, {1.0f, 1.0f}, tMin, tSize, ao, light });
        vertices.push_back({ v3, norm, {0.0f, 1.0f}, tMin, tSize, ao, light });
    }
}

} // namespace

void Player::appendGeometry(std::vector<Vertex>& vertices, const World& world) const {
    // Only render full player model in Third Person perspectives
    if (m_perspective == Perspective::FirstPerson) return;

    const int bx = static_cast<int>(std::floor(m_position.x));
    const int by = static_cast<int>(std::floor(m_position.y + 0.5f));
    const int bz = static_cast<int>(std::floor(m_position.z));
    const float light = world.skyLight(bx, by, bz);
    const float ao = 1.0f;

    // Body transform: Position + Yaw (align local +Z forward with camera yaw where yaw=0 is +X, yaw=90 is +Z)
    glm::mat4 rootMat = glm::translate(glm::mat4(1.0f), m_position);
    rootMat = glm::rotate(rootMat, glm::radians(90.0f - m_yaw), glm::vec3(0, 1, 0));

    const float legSwing = (m_onGround && std::hypot(m_velocity.x, m_velocity.z) > 0.1f)
                           ? std::sin(m_animTime) * 0.45f : 0.0f;
    const float armSwing = legSwing;
    const float punch = std::sin(swingProgress() * 3.14159265f);

    // 1. Torso: 0.48w x 0.72h x 0.24l (Center Y = 1.08, spans [0.72, 1.44])
    addOrientedBox(vertices, {0.0f, 1.08f, 0.0f}, {0.48f, 0.72f, 0.24f}, rootMat,
                   TextureTile::PlayerTorso, TextureTile::PlayerTorso,
                   TextureTile::PlayerTorso, TextureTile::PlayerTorso, light, ao);

    // 2. Left Leg: 0.24w x 0.72h x 0.24l (Hip pivot at Y = 0.72, X = -0.12)
    glm::mat4 lLegMat = glm::translate(rootMat, glm::vec3(-0.12f, 0.72f, 0.0f));
    lLegMat = glm::rotate(lLegMat, legSwing, glm::vec3(1, 0, 0));
    addOrientedBox(vertices, {0.0f, -0.36f, 0.0f}, {0.24f, 0.72f, 0.24f}, lLegMat,
                   TextureTile::PlayerPants, TextureTile::PlayerPants,
                   TextureTile::PlayerShoe, TextureTile::PlayerPants, light, ao);

    // 3. Right Leg: 0.24w x 0.72h x 0.24l (Hip pivot at Y = 0.72, X = +0.12)
    glm::mat4 rLegMat = glm::translate(rootMat, glm::vec3(0.12f, 0.72f, 0.0f));
    rLegMat = glm::rotate(rLegMat, -legSwing, glm::vec3(1, 0, 0));
    addOrientedBox(vertices, {0.0f, -0.36f, 0.0f}, {0.24f, 0.72f, 0.24f}, rLegMat,
                   TextureTile::PlayerPants, TextureTile::PlayerPants,
                   TextureTile::PlayerShoe, TextureTile::PlayerPants, light, ao);

    // 4. Left Arm: 0.24w x 0.72h x 0.24l (Shoulder pivot at Y = 1.44, X = -0.36)
    glm::mat4 lArmMat = glm::translate(rootMat, glm::vec3(-0.36f, 1.44f, 0.0f));
    lArmMat = glm::rotate(lArmMat, -armSwing, glm::vec3(1, 0, 0));
    addOrientedBox(vertices, {0.0f, -0.36f, 0.0f}, {0.24f, 0.72f, 0.24f}, lArmMat,
                   TextureTile::PlayerArm, TextureTile::PlayerArm,
                   TextureTile::PlayerArm, TextureTile::PlayerArm, light, ao);

    // 5. Right Arm: 0.24w x 0.72h x 0.24l (Shoulder pivot at Y = 1.44, X = +0.36)
    float rArmRot = armSwing - punch * 1.3f;
    glm::mat4 rArmMat = glm::translate(rootMat, glm::vec3(0.36f, 1.44f, 0.0f));
    rArmMat = glm::rotate(rArmMat, rArmRot, glm::vec3(1, 0, 0));
    if (punch > 0.01f) rArmMat = glm::rotate(rArmMat, -punch * 0.4f, glm::vec3(0, 1, 0));
    addOrientedBox(vertices, {0.0f, -0.36f, 0.0f}, {0.24f, 0.72f, 0.24f}, rArmMat,
                   TextureTile::PlayerArm, TextureTile::PlayerArm,
                   TextureTile::PlayerArm, TextureTile::PlayerArm, light, ao);

    // 6. Head: 0.48w x 0.48h x 0.48l (Neck at Y = 1.44)
    glm::mat4 headMat = glm::translate(rootMat, glm::vec3(0.0f, 1.44f, 0.0f));
    headMat = glm::rotate(headMat, glm::radians(m_pitch), glm::vec3(1, 0, 0));
    addOrientedBox(vertices, {0.0f, 0.24f, 0.0f}, {0.48f, 0.48f, 0.48f}, headMat,
                   TextureTile::PlayerHead, TextureTile::PlayerHead,
                   TextureTile::PlayerHead, TextureTile::PlayerFace, light, ao);
}

void Player::appendFirstPersonArm(std::vector<Vertex>& vertices, const World& world) const {
    if (m_perspective != Perspective::FirstPerson) return;

    const float sp = swingProgress();
    const float punch = std::sin(sp * 3.14159265f);

    // View bobbing in camera space (smoothly swaying X & Y)
    float bobX = 0.0f;
    float bobY = 0.0f;
    if (m_bobIntensity > 0.001f) {
        bobX = std::sin(m_bobTimer * 0.5f) * 0.018f * m_bobIntensity;
        bobY = -(std::cos(m_bobTimer) * 0.5f + 0.5f) * 0.020f * m_bobIntensity;
    }

    // Camera space viewmodel position: anchored in bottom-right corner looking down -Z
    const glm::vec3 basePos(0.36f + bobX - punch * 0.08f,
                            -0.34f + bobY + punch * 0.06f,
                            -0.48f - punch * 0.10f);

    glm::mat4 armMat = glm::translate(glm::mat4(1.0f), basePos);

    // Minecraft first-person arm orientation in camera space:
    // Base pose: angled slightly inward and forward
    // Punch swing: swings downward-forward and tilts inward
    armMat = glm::rotate(armMat, glm::radians(-16.0f + punch * 45.0f), glm::vec3(1, 0, 0)); // Pitch forward/down
    armMat = glm::rotate(armMat, glm::radians(-24.0f - punch * 26.0f), glm::vec3(0, 1, 0)); // Yaw inward
    armMat = glm::rotate(armMat, glm::radians(punch * 20.0f),          glm::vec3(0, 0, 1)); // Roll inward

    const glm::vec3 eye = eyePosition();
    const int bx = static_cast<int>(std::floor(eye.x));
    const int by = static_cast<int>(std::floor(eye.y));
    const int bz = static_cast<int>(std::floor(eye.z));
    const float light = world.skyLight(bx, by, bz);

    // Arm box in camera space: 0.14w x 0.54h x 0.14l
    addOrientedBox(vertices, {0.0f, -0.22f, 0.0f}, {0.14f, 0.54f, 0.14f}, armMat,
                   TextureTile::PlayerArm, TextureTile::PlayerArm,
                   TextureTile::PlayerArm, TextureTile::PlayerArm, light, 1.0f);
}

} // namespace vox

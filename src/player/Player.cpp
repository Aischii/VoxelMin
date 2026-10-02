#include "player/Player.hpp"
#include "audio/AudioEngine.hpp"
#include "core/Config.hpp"
#include "input/Input.hpp"
#include "world/Block.hpp"
#include "world/World.hpp"
#include "world/BackroomsGenerator.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace vox {

void Player::spawnAt(const World& world, float x, float z) {
    if (world.currentDimension() == DimensionId::Backrooms) {
        const glm::vec3 safe = BackroomsGenerator(world.seed()).findSafeSpawn(const_cast<World&>(world), static_cast<int>(std::floor(x)), static_cast<int>(std::floor(z)));
        setPosition(safe);
        m_velocity = glm::vec3(0.0f);
        m_onGround = true;
        return;
    }

    int wx = static_cast<int>(std::floor(x));
    int wz = static_cast<int>(std::floor(z));
    int surface = world.surfaceHeight(wx, wz);

    // If spawn coordinate is submerged in water, search outward for nearest dry land
    if (surface <= 26) {
        bool foundDry = false;
        for (int r = 1; r <= 32 && !foundDry; ++r) {
            for (int dx = -r; dx <= r && !foundDry; ++dx) {
                for (int dz = -r; dz <= r && !foundDry; ++dz) {
                    if (std::abs(dx) != r && std::abs(dz) != r) continue;
                    const int testX = wx + dx;
                    const int testZ = wz + dz;
                    const int testH = world.surfaceHeight(testX, testZ);
                    if (testH > 26 && isSolid(world.getBlock(testX, testH, testZ))) {
                        wx = testX;
                        wz = testZ;
                        surface = testH;
                        foundDry = true;
                    }
                }
            }
        }
    }

    m_position = glm::vec3(static_cast<float>(wx) + 0.5f, static_cast<float>(surface + 1) + 0.001f, static_cast<float>(wz) + 0.5f);
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

void Player::takeDamage(float amount, const glm::vec3& sourcePos, AudioEngine* audio) {
    if (m_creative || m_flying || m_isDead || m_invulnerableTimer > 0.0f) return;

    m_health = std::max(0.0f, m_health - amount);
    m_invulnerableTimer = 0.45f;
    m_hurtTimer = 0.45f;

    if (audio) {
        audio->play(SoundId::PlayerHurt, 1.0f);
    }

    // Apply knockback impulse away from damage source if provided
    if (glm::length(sourcePos) > 1e-4f) {
        glm::vec3 knock = m_position - sourcePos;
        knock.y = 0.0f;
        if (glm::length(knock) > 1e-4f) {
            knock = glm::normalize(knock);
        } else {
            knock = -m_camera.front();
            knock.y = 0.0f;
            if (glm::length(knock) > 1e-4f) knock = glm::normalize(knock);
        }
        m_velocity.x = knock.x * 6.5f;
        m_velocity.z = knock.z * 6.5f;
        m_velocity.y = 4.2f;
        m_onGround = false;
    }

    if (m_health <= 0.0f) {
        m_health = 0.0f;
        m_isDead = true;
    }
}

void Player::heal(float amount) {
    if (m_isDead) return;
    m_health = std::min(m_maxHealth, m_health + amount);
}

void Player::feed(float hungerGain, float healthGain) {
    if (m_isDead) return;
    m_hunger = std::min(m_maxHunger, m_hunger + hungerGain);
    if (healthGain > 0.0f) {
        heal(healthGain);
    }
}

void Player::drainHunger(float amount) {
    if (m_creative || m_isDead) return;
    m_hunger = std::max(0.0f, m_hunger - amount);
}

void Player::respawn(const World& world, float x, float z) {
    m_health = m_maxHealth;
    m_hunger = m_maxHunger;
    m_oxygen = m_maxOxygen;
    m_fallDistance = 0.0f;
    m_invulnerableTimer = 0.0f;
    m_hurtTimer = 0.0f;
    m_regenTimer = 0.0f;
    m_starveTimer = 0.0f;
    m_drownTimer = 0.0f;
    m_isDead = false;
    spawnAt(world, x, z);
}

void Player::update(float dt, const Input& input, const World& world, AudioEngine* audio) {
    if (m_invulnerableTimer > 0.0f) m_invulnerableTimer -= dt;
    if (m_hurtTimer > 0.0f) m_hurtTimer -= dt;

    if (m_creative) {
        m_health = m_maxHealth;
        m_hunger = m_maxHunger;
        m_oxygen = m_maxOxygen;
        m_fallDistance = 0.0f;
        m_isDead = false;
    }

    applyLook(input);

    m_timeSinceWPress += dt;

    if (input.cursorCaptured() && !m_isDead) {
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

    if (!input.cursorCaptured() || !input.keyDown(GLFW_KEY_W) || m_isDead) {
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
    if (input.cursorCaptured() && !m_isDead) {
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
    const int byHead = static_cast<int>(std::floor(m_position.y + config::PLAYER_EYE_HEIGHT));
    const int bz = static_cast<int>(std::floor(m_position.z));
    const bool wasInWater = m_inWater;
    m_inWater = (isLiquid(world.getBlock(bx, byFeet, bz)) || isLiquid(world.getBlock(bx, byMid, bz)));
    const bool headSubmerged = isLiquid(world.getBlock(bx, byHead, bz));

    if (!wasInWater && m_inWater && audio) {
        audio->play(SoundId::WaterSplash, 0.75f);
    }

    // Oxygen & Drowning
    if (headSubmerged && !m_creative && !m_isDead) {
        m_oxygen = std::max(0.0f, m_oxygen - dt);
        if (m_oxygen <= 0.0f) {
            m_drownTimer += dt;
            if (m_drownTimer >= 1.0f) {
                m_drownTimer = 0.0f;
                takeDamage(10.0f, glm::vec3(0.0f), audio);
            }
        } else {
            m_drownTimer = 0.0f;
        }
    } else {
        m_oxygen = std::min(m_maxOxygen, m_oxygen + dt * 8.0f);
        m_drownTimer = 0.0f;
    }

    // Hunger drain
    if (!m_creative && !m_isDead) {
        drainHunger(dt * 0.035f);
        if (m_sprinting && glm::length(wish) > 0.1f) {
            drainHunger(dt * 0.20f);
        }
    }

    // Natural regeneration
    if (!m_creative && m_hunger >= 90.0f && m_health < m_maxHealth && !m_isDead) {
        m_regenTimer += dt;
        if (m_regenTimer >= 2.5f) {
            m_regenTimer = 0.0f;
            heal(5.0f);
            drainHunger(3.0f);
        }
    } else {
        m_regenTimer = 0.0f;
    }

    // Starvation
    if (!m_creative && m_hunger <= 0.0f && !m_isDead) {
        m_starveTimer += dt;
        if (m_starveTimer >= 3.0f) {
            m_starveTimer = 0.0f;
            takeDamage(5.0f, glm::vec3(0.0f), audio);
        }
    } else {
        m_starveTimer = 0.0f;
    }

    // Double-space detection for creative flight toggle
    m_timeSinceSpacePress += dt;
    if (m_creative && input.cursorCaptured() && !m_isDead) {
        if (input.keyPressed(GLFW_KEY_SPACE)) {
            if (m_timeSinceSpacePress < 0.28f) {
                toggleFlying();
                m_timeSinceSpacePress = 100.0f;
            } else {
                m_timeSinceSpacePress = 0.0f;
            }
        }
    }

    // Jump queueing & auto-bunnyhop
    if (input.cursorCaptured() && !m_isDead) {
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
        if (input.cursorCaptured() && !m_isDead) {
            if (input.keyDown(GLFW_KEY_SPACE))      m_velocity.y = speed;
            if (input.keyDown(GLFW_KEY_LEFT_SHIFT)) m_velocity.y = -speed;
        }
        m_fallDistance = 0.0f;
    } else if (m_inWater) {
        m_fallDistance = 0.0f;
        // Water swimming & buoyancy physics
        const float waterSpeed = WALK_SPEED * 0.70f;
        m_velocity.x = wish.x * waterSpeed;
        m_velocity.z = wish.z * waterSpeed;

        if (input.cursorCaptured() && !m_isDead && input.keyDown(GLFW_KEY_SPACE)) {
            m_velocity.y = 4.2f; // Active upward swimming
        } else if (input.cursorCaptured() && !m_isDead && input.keyDown(GLFW_KEY_LEFT_SHIFT)) {
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
            drainHunger(0.12f);
        } else {
            if (glm::length(wish) > 1e-4f) {
                // Snappy ground acceleration towards wish direction
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
        // Air physics: accumulate downward fall distance
        if (m_velocity.y < 0.0f) {
            m_fallDistance += -m_velocity.y * dt;
        }

        if (glm::length(wish) > 1e-4f) {
            const float airWishSpeed = m_sprinting ? SPRINT_SPEED : WALK_SPEED;
            const glm::vec3 targetAirVel = wish * airWishSpeed;
            m_velocity.x = glm::mix(m_velocity.x, targetAirVel.x, std::min(1.0f, dt * AIR_ACCEL_RATE));
            m_velocity.z = glm::mix(m_velocity.z, targetAirVel.z, std::min(1.0f, dt * AIR_ACCEL_RATE));
        } else {
            m_velocity.x = glm::mix(m_velocity.x, 0.0f, std::min(1.0f, dt * AIR_DECEL_RATE));
            m_velocity.z = glm::mix(m_velocity.z, 0.0f, std::min(1.0f, dt * AIR_DECEL_RATE));
        }

        // Gravity
        m_velocity.y -= GRAVITY * dt;
        if (m_velocity.y < -50.0f) m_velocity.y = -50.0f;
    }

    const bool wasInAir = !m_onGround && !m_inWater && !m_flying;
    m_onGround = false;
    moveAxis(world, m_velocity.x * dt, 0);
    moveAxis(world, m_velocity.y * dt, 1);
    moveAxis(world, m_velocity.z * dt, 2);

    if (wasInAir && m_onGround && !m_inWater && !m_flying) {
        if (m_fallDistance > 3.5f && !m_creative && !m_isDead) {
            const float fallDmg = (m_fallDistance - 3.5f) * 11.0f;
            takeDamage(fallDmg, glm::vec3(0.0f), audio);
        }
        m_fallDistance = 0.0f;
    }

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

    // View bobbing & Presence Footsteps stride accumulation
    float targetBobIntensity = 0.0f;
    if (m_onGround && !m_flying && !m_inWater && hSpeed > 0.2f) {
        targetBobIntensity = m_sprinting ? 1.25f : 1.0f;
        const float freqMult = m_sprinting ? 1.20f : 1.0f;
        m_bobTimer += hSpeed * dt * 2.85f * freqMult;

        m_stepDistance += hSpeed * dt;
        const float stepInterval = m_sprinting ? 1.65f : 2.05f;
        if (m_stepDistance >= stepInterval) {
            m_stepDistance = 0.0f;
            if (audio) {
                const int bx = static_cast<int>(std::floor(m_position.x));
                const int by = static_cast<int>(std::floor(m_position.y - 0.2f));
                const int bz = static_cast<int>(std::floor(m_position.z));
                const BlockId blockBelow = world.getBlock(bx, by, bz);
                SoundId stepSnd = SoundId::StepGrass;
                if (m_inWater) {
                    stepSnd = SoundId::WaterFlow;
                } else if (blockBelow == BlockId::Wood || blockBelow == BlockId::WoodX ||
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
    m_bobIntensity = glm::mix(m_bobIntensity, targetBobIntensity, std::min(1.0f, dt * 7.5f));

    float bobX = 0.0f;
    float bobY = 0.0f;
    float bobPitch = 0.0f;
    float bobRoll = 0.0f;

    if (m_bobIntensity > 0.001f && m_bobbingEnabled) {
        // Lateral sway (sinusoidal) and vertical footfall dip (absolute sine/cosine)
        bobX = std::sin(m_bobTimer * 0.5f) * 0.026f * m_bobIntensity;
        bobY = -std::abs(std::sin(m_bobTimer * 0.5f)) * 0.030f * m_bobIntensity;
        bobRoll = std::sin(m_bobTimer * 0.5f) * 0.45f * m_bobIntensity;
        bobPitch = -std::abs(std::cos(m_bobTimer * 0.5f)) * 0.30f * m_bobIntensity;
    }

    m_roll = bobRoll;

    const glm::vec3 eyePos = eyePosition();

    if (m_perspective == Perspective::FirstPerson) {
        const glm::vec3 cameraPos = eyePos + right * bobX + glm::vec3(0.0f, bobY, 0.0f);
        m_camera.setPosition(cameraPos);
        m_camera.setRotation(m_yaw, m_pitch + bobPitch, m_roll);
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
                    float torchLight,
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

        vertices.push_back({ v0, norm, {0.0f, 0.0f}, tMin, tSize, ao, light, torchLight });
        vertices.push_back({ v1, norm, {1.0f, 0.0f}, tMin, tSize, ao, light, torchLight });
        vertices.push_back({ v2, norm, {1.0f, 1.0f}, tMin, tSize, ao, light, torchLight });

        vertices.push_back({ v0, norm, {0.0f, 0.0f}, tMin, tSize, ao, light, torchLight });
        vertices.push_back({ v2, norm, {1.0f, 1.0f}, tMin, tSize, ao, light, torchLight });
        vertices.push_back({ v3, norm, {0.0f, 1.0f}, tMin, tSize, ao, light, torchLight });
    }
}

void addOrientedItemSprite(std::vector<Vertex>& vertices,
                           const glm::vec3& center,
                           float size,
                           const glm::mat4& transform,
                           TextureTile tile,
                           float light,
                           float torchLight,
                           float ao) {
    const glm::vec2 tMin = tileMinUV(tile);
    const glm::vec2 tSize(1.0f / 16.0f, 1.0f / 16.0f);
    const float hs = size * 0.5f;
    const float thick = 0.008f; // Sleek thin item thickness

    // Front Face (+Z)
    const glm::vec3 f0 = glm::vec3(transform * glm::vec4(center + glm::vec3(-hs, -hs,  thick), 1.0f));
    const glm::vec3 f1 = glm::vec3(transform * glm::vec4(center + glm::vec3( hs, -hs,  thick), 1.0f));
    const glm::vec3 f2 = glm::vec3(transform * glm::vec4(center + glm::vec3( hs,  hs,  thick), 1.0f));
    const glm::vec3 f3 = glm::vec3(transform * glm::vec4(center + glm::vec3(-hs,  hs,  thick), 1.0f));
    const glm::vec3 normF = glm::normalize(glm::mat3(transform) * glm::vec3(0, 0, 1));

    vertices.push_back({ f0, normF, {0.0f, 0.0f}, tMin, tSize, ao, light, torchLight });
    vertices.push_back({ f1, normF, {1.0f, 0.0f}, tMin, tSize, ao, light, torchLight });
    vertices.push_back({ f2, normF, {1.0f, 1.0f}, tMin, tSize, ao, light, torchLight });

    vertices.push_back({ f0, normF, {0.0f, 0.0f}, tMin, tSize, ao, light, torchLight });
    vertices.push_back({ f2, normF, {1.0f, 1.0f}, tMin, tSize, ao, light, torchLight });
    vertices.push_back({ f3, normF, {0.0f, 1.0f}, tMin, tSize, ao, light, torchLight });

    // Back Face (-Z)
    const glm::vec3 b0 = glm::vec3(transform * glm::vec4(center + glm::vec3(-hs, -hs, -thick), 1.0f));
    const glm::vec3 b1 = glm::vec3(transform * glm::vec4(center + glm::vec3( hs, -hs, -thick), 1.0f));
    const glm::vec3 b2 = glm::vec3(transform * glm::vec4(center + glm::vec3( hs,  hs, -thick), 1.0f));
    const glm::vec3 b3 = glm::vec3(transform * glm::vec4(center + glm::vec3(-hs,  hs, -thick), 1.0f));
    const glm::vec3 normB = glm::normalize(glm::mat3(transform) * glm::vec3(0, 0, -1));

    vertices.push_back({ b1, normB, {1.0f, 0.0f}, tMin, tSize, ao, light, torchLight });
    vertices.push_back({ b0, normB, {0.0f, 0.0f}, tMin, tSize, ao, light, torchLight });
    vertices.push_back({ b3, normB, {0.0f, 1.0f}, tMin, tSize, ao, light, torchLight });

    vertices.push_back({ b1, normB, {1.0f, 0.0f}, tMin, tSize, ao, light, torchLight });
    vertices.push_back({ b3, normB, {0.0f, 1.0f}, tMin, tSize, ao, light, torchLight });
    vertices.push_back({ b2, normB, {1.0f, 1.0f}, tMin, tSize, ao, light, torchLight });

    // Top Rim (+Y)
    const glm::vec3 normT = glm::normalize(glm::mat3(transform) * glm::vec3(0, 1, 0));
    vertices.push_back({ f3, normT, {0.0f, 1.0f}, tMin, tSize, ao * 0.95f, light, torchLight });
    vertices.push_back({ f2, normT, {1.0f, 1.0f}, tMin, tSize, ao * 0.95f, light, torchLight });
    vertices.push_back({ b2, normT, {1.0f, 1.0f}, tMin, tSize, ao * 0.95f, light, torchLight });

    vertices.push_back({ f3, normT, {0.0f, 1.0f}, tMin, tSize, ao * 0.95f, light, torchLight });
    vertices.push_back({ b2, normT, {1.0f, 1.0f}, tMin, tSize, ao * 0.95f, light, torchLight });
    vertices.push_back({ b3, normT, {0.0f, 1.0f}, tMin, tSize, ao * 0.95f, light, torchLight });

    // Bottom Rim (-Y)
    const glm::vec3 normD = glm::normalize(glm::mat3(transform) * glm::vec3(0, -1, 0));
    vertices.push_back({ b0, normD, {0.0f, 0.0f}, tMin, tSize, ao * 0.75f, light, torchLight });
    vertices.push_back({ b1, normD, {1.0f, 0.0f}, tMin, tSize, ao * 0.75f, light, torchLight });
    vertices.push_back({ f1, normD, {1.0f, 0.0f}, tMin, tSize, ao * 0.75f, light, torchLight });

    vertices.push_back({ b0, normD, {0.0f, 0.0f}, tMin, tSize, ao * 0.75f, light, torchLight });
    vertices.push_back({ f1, normD, {1.0f, 0.0f}, tMin, tSize, ao * 0.75f, light, torchLight });
    vertices.push_back({ f0, normD, {0.0f, 0.0f}, tMin, tSize, ao * 0.75f, light, torchLight });

    // Right Rim (+X)
    const glm::vec3 normR = glm::normalize(glm::mat3(transform) * glm::vec3(1, 0, 0));
    vertices.push_back({ f1, normR, {1.0f, 0.0f}, tMin, tSize, ao * 0.85f, light, torchLight });
    vertices.push_back({ b1, normR, {1.0f, 0.0f}, tMin, tSize, ao * 0.85f, light, torchLight });
    vertices.push_back({ b2, normR, {1.0f, 1.0f}, tMin, tSize, ao * 0.85f, light, torchLight });

    vertices.push_back({ f1, normR, {1.0f, 0.0f}, tMin, tSize, ao * 0.85f, light, torchLight });
    vertices.push_back({ b2, normR, {1.0f, 1.0f}, tMin, tSize, ao * 0.85f, light, torchLight });
    vertices.push_back({ f2, normR, {1.0f, 1.0f}, tMin, tSize, ao * 0.85f, light, torchLight });

    // Left Rim (-X)
    const glm::vec3 normL = glm::normalize(glm::mat3(transform) * glm::vec3(-1, 0, 0));
    vertices.push_back({ b0, normL, {0.0f, 0.0f}, tMin, tSize, ao * 0.85f, light, torchLight });
    vertices.push_back({ f0, normL, {0.0f, 0.0f}, tMin, tSize, ao * 0.85f, light, torchLight });
    vertices.push_back({ f3, normL, {0.0f, 1.0f}, tMin, tSize, ao * 0.85f, light, torchLight });

    vertices.push_back({ b0, normL, {0.0f, 0.0f}, tMin, tSize, ao * 0.85f, light, torchLight });
    vertices.push_back({ f3, normL, {0.0f, 1.0f}, tMin, tSize, ao * 0.85f, light, torchLight });
    vertices.push_back({ b3, normL, {0.0f, 1.0f}, tMin, tSize, ao * 0.85f, light, torchLight });
}

bool isSolidBlockItem(BlockId id) {
    return id == BlockId::Grass || id == BlockId::Dirt || id == BlockId::Stone ||
           id == BlockId::Wood || id == BlockId::Leaves || id == BlockId::Sand ||
           id == BlockId::Bedrock || id == BlockId::Planks || id == BlockId::Cobblestone ||
           id == BlockId::CoalOre || id == BlockId::IronOre || id == BlockId::GoldOre ||
           id == BlockId::DiamondOre || id == BlockId::DirtPath || id == BlockId::WoodX ||
           id == BlockId::WoodZ || id == BlockId::CraftingTable;
}

} // namespace

void Player::appendGeometry(std::vector<Vertex>& vertices, const World& world) const {
    // Only render full player model in Third Person perspectives
    if (m_perspective == Perspective::FirstPerson) return;

    const int bx = static_cast<int>(std::floor(m_position.x));
    const int by = static_cast<int>(std::floor(m_position.y + 0.5f));
    const int bz = static_cast<int>(std::floor(m_position.z));
    const float light = static_cast<float>(world.getSunLight(bx, by, bz)) / 15.0f;
    const float torchLight = static_cast<float>(world.getBlockLight(bx, by, bz)) / 15.0f;
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
                   TextureTile::PlayerTorso, TextureTile::PlayerTorso, light, torchLight, ao);

    // 2. Left Leg: 0.24w x 0.72h x 0.24l (Hip pivot at Y = 0.72, X = -0.12)
    glm::mat4 lLegMat = glm::translate(rootMat, glm::vec3(-0.12f, 0.72f, 0.0f));
    lLegMat = glm::rotate(lLegMat, legSwing, glm::vec3(1, 0, 0));
    addOrientedBox(vertices, {0.0f, -0.36f, 0.0f}, {0.24f, 0.72f, 0.24f}, lLegMat,
                   TextureTile::PlayerPants, TextureTile::PlayerPants,
                   TextureTile::PlayerShoe, TextureTile::PlayerPants, light, torchLight, ao);

    // 3. Right Leg: 0.24w x 0.72h x 0.24l (Hip pivot at Y = 0.72, X = +0.12)
    glm::mat4 rLegMat = glm::translate(rootMat, glm::vec3(0.12f, 0.72f, 0.0f));
    rLegMat = glm::rotate(rLegMat, -legSwing, glm::vec3(1, 0, 0));
    addOrientedBox(vertices, {0.0f, -0.36f, 0.0f}, {0.24f, 0.72f, 0.24f}, rLegMat,
                   TextureTile::PlayerPants, TextureTile::PlayerPants,
                   TextureTile::PlayerShoe, TextureTile::PlayerPants, light, torchLight, ao);

    // 4. Left Arm: 0.24w x 0.72h x 0.24l (Shoulder pivot at Y = 1.44, X = -0.36)
    glm::mat4 lArmMat = glm::translate(rootMat, glm::vec3(-0.36f, 1.44f, 0.0f));
    lArmMat = glm::rotate(lArmMat, -armSwing, glm::vec3(1, 0, 0));
    addOrientedBox(vertices, {0.0f, -0.36f, 0.0f}, {0.24f, 0.72f, 0.24f}, lArmMat,
                   TextureTile::PlayerArm, TextureTile::PlayerArm,
                   TextureTile::PlayerArm, TextureTile::PlayerArm, light, torchLight, ao);

    // 5. Right Arm: 0.24w x 0.72h x 0.24l (Shoulder pivot at Y = 1.44, X = +0.36)
    float rArmRot = armSwing - punch * 1.3f;
    glm::mat4 rArmMat = glm::translate(rootMat, glm::vec3(0.36f, 1.44f, 0.0f));
    rArmMat = glm::rotate(rArmMat, rArmRot, glm::vec3(1, 0, 0));
    if (punch > 0.01f) rArmMat = glm::rotate(rArmMat, -punch * 0.4f, glm::vec3(0, 1, 0));
    addOrientedBox(vertices, {0.0f, -0.36f, 0.0f}, {0.24f, 0.72f, 0.24f}, rArmMat,
                   TextureTile::PlayerArm, TextureTile::PlayerArm,
                   TextureTile::PlayerArm, TextureTile::PlayerArm, light, torchLight, ao);

    // 6. Head: 0.48w x 0.48h x 0.48l (Neck at Y = 1.44)
    glm::mat4 headMat = glm::translate(rootMat, glm::vec3(0.0f, 1.44f, 0.0f));
    headMat = glm::rotate(headMat, glm::radians(-m_pitch), glm::vec3(1, 0, 0));
    addOrientedBox(vertices, {0.0f, 0.24f, 0.0f}, {0.48f, 0.48f, 0.48f}, headMat,
                   TextureTile::PlayerHead, TextureTile::PlayerHead,
                   TextureTile::PlayerHead, TextureTile::PlayerFace, light, torchLight, ao);
}

void Player::appendFirstPersonArm(std::vector<Vertex>& vertices, const World& world) const {
    if (m_perspective != Perspective::FirstPerson) return;

    const float sp = swingProgress();
    const float sinSp = std::sin(std::sqrt(sp) * 3.14159265f);
    const float punch = std::sin(sp * 3.14159265f);

    // Hand bobbing in camera space (smoothly swaying X, Y & Z with inertia)
    float handBobX = 0.0f;
    float handBobY = 0.0f;
    float handBobZ = 0.0f;
    float handRoll = 0.0f;
    float handPitch = 0.0f;

    // Hand bobbing scales smoothly with locomotion (gentle rhythm maintained if view bobbing disabled)
    const float armIntensity = m_bobbingEnabled ? m_bobIntensity : (m_bobIntensity * 0.35f);
    if (armIntensity > 0.001f) {
        handBobX = -std::sin(m_bobTimer * 0.5f) * 0.024f * armIntensity;
        handBobY = -std::abs(std::sin(m_bobTimer * 0.5f)) * 0.020f * armIntensity;
        handBobZ = std::cos(m_bobTimer * 0.5f) * 0.015f * armIntensity;
        handRoll = std::sin(m_bobTimer * 0.5f) * 3.5f * armIntensity;
        handPitch = std::abs(std::sin(m_bobTimer * 0.5f)) * 3.0f * armIntensity;
    }

    const glm::vec3 eye = eyePosition();
    const int bx = static_cast<int>(std::floor(eye.x));
    const int by = static_cast<int>(std::floor(eye.y));
    const int bz = static_cast<int>(std::floor(eye.z));
    const float light = std::max(0.08f, static_cast<float>(world.getSunLight(bx, by, bz)) / 15.0f);
    const float torchLight = static_cast<float>(world.getBlockLight(bx, by, bz)) / 15.0f;

    if (m_heldItem == BlockId::Air) {
        // --- 1. Empty Hand (Longer, authentic Minecraft first-person arm) ---
        // Base anchor in bottom-right corner of screen
        const glm::vec3 basePos(0.38f + handBobX - sinSp * 0.15f,
                                -0.28f + handBobY - sinSp * 0.10f,
                                -0.36f + handBobZ - punch * 0.20f);

        glm::mat4 armMat = glm::translate(glm::mat4(1.0f), basePos);
        // Angles: tilted forward-up, angled inward toward center, slight wrist roll
        armMat = glm::rotate(armMat, glm::radians(-40.0f - handPitch - sinSp * 45.0f), glm::vec3(1, 0, 0));
        armMat = glm::rotate(armMat, glm::radians(-18.0f + sinSp * 25.0f), glm::vec3(0, 1, 0));
        armMat = glm::rotate(armMat, glm::radians(18.0f + handRoll - sinSp * 30.0f),  glm::vec3(0, 0, 1));

        // Long forearm and hand (Steve skin tone entering from bottom-right)
        addOrientedBox(vertices, {0.0f, 0.0f, -0.26f}, {0.14f, 0.14f, 0.70f}, armMat,
                       TextureTile::PlayerSkin, TextureTile::PlayerSkin,
                       TextureTile::PlayerSkin, TextureTile::PlayerSkin, light, torchLight, 1.0f);
    } else {
        const BlockDef& def = blockDef(m_heldItem);

        if (isTorch(m_heldItem)) {
            // --- 2. Held Torch (Upright 3D stick in hand) ---
            const glm::vec3 basePos(0.32f + handBobX - sinSp * 0.15f,
                                    -0.24f + handBobY - sinSp * 0.10f,
                                    -0.38f + handBobZ - punch * 0.18f);

            glm::mat4 toolMat = glm::translate(glm::mat4(1.0f), basePos);
            toolMat = glm::rotate(toolMat, glm::radians(-18.0f - handPitch - sinSp * 65.0f), glm::vec3(1, 0, 0));
            toolMat = glm::rotate(toolMat, glm::radians(-24.0f + sinSp * 30.0f), glm::vec3(0, 1, 0));
            toolMat = glm::rotate(toolMat, glm::radians(20.0f + handRoll - sinSp * 35.0f),  glm::vec3(0, 0, 1));

            // Forearm and hand gripping torch
            addOrientedBox(vertices, {0.0f, -0.06f, 0.16f}, {0.12f, 0.12f, 0.52f}, toolMat,
                           TextureTile::PlayerSkin, TextureTile::PlayerSkin,
                           TextureTile::PlayerSkin, TextureTile::PlayerSkin, light, torchLight, 1.0f);

            // Upright 3D Torch
            addOrientedBox(vertices, {0.0f, 0.14f, -0.06f}, {0.0625f, 0.48f, 0.0625f}, toolMat,
                           TextureTile::Planks, TextureTile::Planks,
                           TextureTile::Planks, TextureTile::Planks, light, torchLight, 1.0f);
            addOrientedBox(vertices, {0.0f, 0.34f, -0.06f}, {0.075f, 0.12f, 0.075f}, toolMat,
                           TextureTile::Torch, TextureTile::Torch,
                           TextureTile::Torch, TextureTile::Torch, light, torchLight, 1.0f);
        } else if (!isSolidBlockItem(m_heldItem)) {
            // --- 3. Held 2.5D Item Sprite (Tools, Weapons, Food, Materials) ---
            const glm::vec3 basePos(0.32f + handBobX - sinSp * 0.16f,
                                    -0.22f + handBobY - sinSp * 0.12f,
                                    -0.36f + handBobZ - punch * 0.18f);

            glm::mat4 toolMat = glm::translate(glm::mat4(1.0f), basePos);
            toolMat = glm::rotate(toolMat, glm::radians(-15.0f - handPitch - sinSp * 72.0f), glm::vec3(1, 0, 0));
            toolMat = glm::rotate(toolMat, glm::radians(-25.0f + sinSp * 35.0f), glm::vec3(0, 1, 0));
            toolMat = glm::rotate(toolMat, glm::radians(15.0f + handRoll - sinSp * 38.0f),  glm::vec3(0, 0, 1));

            // Forearm and hand gripping handle
            addOrientedBox(vertices, {0.0f, -0.06f, 0.16f}, {0.12f, 0.12f, 0.52f}, toolMat,
                           TextureTile::PlayerSkin, TextureTile::PlayerSkin,
                           TextureTile::PlayerSkin, TextureTile::PlayerSkin, light, torchLight, 1.0f);

            // 2.5D Extruded Item Sprite (Handle in hand, head pointing top-left)
            glm::mat4 spriteMat = glm::translate(toolMat, glm::vec3(-0.04f, 0.14f, -0.06f));
            spriteMat = glm::rotate(spriteMat, glm::radians(90.0f), glm::vec3(0, 0, 1));
            spriteMat = glm::rotate(spriteMat, glm::radians(-12.0f), glm::vec3(0, 1, 0));
            addOrientedItemSprite(vertices, glm::vec3(0.0f), 0.38f, spriteMat,
                                 def.side, light, torchLight, 1.0f);
        } else {
            // --- 4. Held 3D Block (Isometric tilt) ---
            const glm::vec3 basePos(0.30f + handBobX - sinSp * 0.12f,
                                    -0.22f + handBobY - sinSp * 0.08f,
                                    -0.36f + handBobZ - punch * 0.14f);

            glm::mat4 blockMat = glm::translate(glm::mat4(1.0f), basePos);
            blockMat = glm::rotate(blockMat, glm::radians(18.0f - handPitch - sinSp * 36.0f), glm::vec3(1, 0, 0));
            blockMat = glm::rotate(blockMat, glm::radians(38.0f + sinSp * 24.0f), glm::vec3(0, 1, 0));
            blockMat = glm::rotate(blockMat, glm::radians(-12.0f + handRoll + sinSp * 20.0f), glm::vec3(0, 0, 1));

            // Forearm and hand supporting beneath
            addOrientedBox(vertices, {0.02f, -0.10f, 0.16f}, {0.12f, 0.12f, 0.50f}, blockMat,
                           TextureTile::PlayerSkin, TextureTile::PlayerSkin,
                           TextureTile::PlayerSkin, TextureTile::PlayerSkin, light, torchLight, 1.0f);

            // Held 3D Mini Block
            const glm::vec3 blockSize(0.20f);
            addOrientedBox(vertices, {0.0f, 0.06f, -0.02f}, blockSize, blockMat,
                           def.top, def.side, def.bottom, def.side, light, torchLight, 1.0f);
        }
    }
}

} // namespace vox

#include "entity/Mob.hpp"
#include "core/Config.hpp"
#include "world/Block.hpp"
#include "world/World.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace vox {
namespace {

constexpr float GRAVITY = 24.0f;

float wrapAngle(float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

float randomFloat(float minVal, float maxVal) {
    const float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    return minVal + r * (maxVal - minVal);
}

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

    // Box corners in local space
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

        // Transform corners
        const glm::vec3 v0 = glm::vec3(transform * glm::vec4(center + f.c1, 1.0f));
        const glm::vec3 v1 = glm::vec3(transform * glm::vec4(center + f.c2, 1.0f));
        const glm::vec3 v2 = glm::vec3(transform * glm::vec4(center + f.c3, 1.0f));
        const glm::vec3 v3 = glm::vec3(transform * glm::vec4(center + f.c4, 1.0f));
        const glm::vec3 norm = glm::normalize(glm::mat3(transform) * f.normal);

        // Quad triangles: (v0, v1, v2) and (v0, v2, v3)
        vertices.push_back({ v0, norm, {0.0f, 0.0f}, tMin, tSize, ao, light, torchLight });
        vertices.push_back({ v1, norm, {1.0f, 0.0f}, tMin, tSize, ao, light, torchLight });
        vertices.push_back({ v2, norm, {1.0f, 1.0f}, tMin, tSize, ao, light, torchLight });

        vertices.push_back({ v0, norm, {0.0f, 0.0f}, tMin, tSize, ao, light, torchLight });
        vertices.push_back({ v2, norm, {1.0f, 1.0f}, tMin, tSize, ao, light, torchLight });
        vertices.push_back({ v3, norm, {0.0f, 1.0f}, tMin, tSize, ao, light, torchLight });
    }
}

} // namespace

Mob::Mob(MobType type, const glm::vec3& position, float yaw)
    : m_type(type), m_position(position), m_yaw(yaw), m_targetYaw(yaw) {
    if (type == MobType::PigmanVillager) {
        m_health = 20;
    } else if (type == MobType::Cow) {
        m_health = 15;
    } else {
        m_health = 10;
    }
    m_stateTimer = randomFloat(2.0f, 5.0f);
    m_goals = aiProfileFor(type).goals;
    updateForwardVector();
}

float Mob::halfWidth() const {
    if (m_type == MobType::PigmanVillager) return 0.35f;
    return (m_type == MobType::Pig) ? 0.35f : 0.42f;
}

float Mob::halfHeight() const {
    if (m_type == MobType::PigmanVillager) return 0.95f;
    return (m_type == MobType::Pig) ? 0.45f : 0.70f;
}

void Mob::alertAggro(const glm::vec3& targetPos) {
    if (m_type == MobType::PigmanVillager && m_state != MobState::Hostile) {
        m_state = MobState::Hostile;
        m_aggroTimer = 10.0f;
        m_panicSource = targetPos;
        m_blockedTimer = 0.0f;
    }
}

void Mob::takeDamage(int amount, const glm::vec3& sourcePos) {
    m_health -= amount;
    m_hurtTimer = 0.45f;
    m_knockbackTimer = 0.35f;
    m_panicSource = sourcePos;

    if (m_goals & AiRetaliate) {
        // Pigman Villager gets angered and will retaliate after knockback recoil
        m_state = MobState::Hostile;
        m_aggroTimer = 12.0f; // 12 seconds interest window
    } else if (m_goals & AiPanic) {
        m_state = MobState::Panic;
        m_stateTimer = randomFloat(3.5f, 5.0f);
    } else {
        m_state = MobState::Idle;
        m_stateTimer = randomFloat(2.0f, 3.0f);
    }
    m_blockedTimer = 0.0f;

    // Apply knockback impulse (away from attacker)
    glm::vec3 knockDir = m_position - sourcePos;
    knockDir.y = 0.0f;
    if (glm::length(knockDir) > 1e-4f) {
        knockDir = glm::normalize(knockDir);
    } else {
        knockDir = glm::vec3(0, 0, 1);
    }
    m_velocity.x = knockDir.x * 7.5f;
    m_velocity.z = knockDir.z * 7.5f;
    m_velocity.y = 4.8f;
    m_onGround = false;
}

bool Mob::collidesWithRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDist, float& outDist) const {
    const float hw = halfWidth() + 0.1f;
    const float hh = halfHeight() * 2.0f;

    const glm::vec3 bMin(m_position.x - hw, m_position.y, m_position.z - hw);
    const glm::vec3 bMax(m_position.x + hw, m_position.y + hh, m_position.z + hw);

    float tMin = 0.0f;
    float tMax = maxDist;

    for (int i = 0; i < 3; ++i) {
        if (std::abs(rayDir[i]) < 1e-6f) {
            if (rayOrigin[i] < bMin[i] || rayOrigin[i] > bMax[i]) return false;
        } else {
            float t1 = (bMin[i] - rayOrigin[i]) / rayDir[i];
            float t2 = (bMax[i] - rayOrigin[i]) / rayDir[i];
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            if (tMin > tMax) return false;
        }
    }

    outDist = tMin;
    return tMin <= maxDist;
}

bool Mob::collides(const World& world, const glm::vec3& feet) const {
    const float hw = halfWidth();
    const float hh = halfHeight() * 2.0f;

    const glm::vec3 minP(feet.x - hw, feet.y, feet.z - hw);
    const glm::vec3 maxP(feet.x + hw, feet.y + hh, feet.z + hw);

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

void Mob::updateForwardVector() {
    // Only recompute when the heading actually changed: this is the mob-side
    // equivalent of the mod's cached-trig lookups.
    if (std::abs(wrapAngle(m_yaw - m_cachedYaw)) <= 0.01f) return;
    const float rad = glm::radians(m_yaw);
    m_fwdX = std::sin(rad);
    m_fwdZ = std::cos(rad);
    m_cachedYaw = m_yaw;
}

bool Mob::tryStepUp(const World& world, const glm::vec3& candidate) {
    // Walk (not jump) up a 1-block ledge: raise the whole body by one step and
    // re-test. This is what lets mobs climb terrain without losing momentum.
    glm::vec3 stepped = candidate;
    stepped.y += kStepHeight;
    if (collides(world, stepped)) return false;
    m_position = stepped;
    m_stepUps++;
    return true;
}

int Mob::dropAhead(const World& world, float distance) const {
    // Probes the column `distance` blocks ahead along the facing direction and
    // returns how far the ground drops there: 0 means flat or a wall, n means an
    // n-block drop, and kSafeDrop+1 means "no ground within range" (a cliff).
    const int x = static_cast<int>(std::floor(m_position.x + m_fwdX * distance));
    const int z = static_cast<int>(std::floor(m_position.z + m_fwdZ * distance));
    const int y0 = static_cast<int>(std::floor(m_position.y));

    for (int i = 0; i <= kSafeDrop + 1; ++i) {
        if (isSolid(world.getBlock(x, y0 - i, z))) return i;
    }
    return kSafeDrop + 1;
}

bool Mob::tryJump() {
    if (!(m_goals & AiJump)) return false;
    if (m_jumpCooldown > 0.0f) return false;
    m_velocity.y = kJumpSpeed;
    m_onGround = false;
    m_jumpCooldown = kJumpCooldown;
    m_jumps++;
    return true;
}

void Mob::rerollHeading() {
    // A mob that has been walking into something needs a new plan, otherwise it
    // grinds against the same wall until its wander timer expires.
    m_targetYaw = wrapAngle(m_yaw + randomFloat(120.0f, 240.0f));
    m_stateTimer = std::max(m_stateTimer, randomFloat(1.5f, 3.0f));
}

void Mob::moveAxis(const World& world, float delta, int axis) {
    if (delta == 0.0f) return;

    glm::vec3 candidate = m_position;
    candidate[axis] += delta;

    if (!collides(world, candidate)) {
        m_position = candidate;
        return;
    }

    if (axis == 1) {
        if (delta < 0.0f) m_onGround = true;
        m_velocity.y = 0.0f;
        return;
    }

    // Horizontal move was blocked: if grounded, launch upward into a natural jump arc
    // to clear the ledge.
    if (m_onGround) {
        if ((m_goals & AiStepUp) && tryStepUp(world, candidate)) {
            m_velocity[axis] = 0.0f;
            return;
        }
        tryJump();
    }

    m_velocity[axis] = 0.0f;
    m_blockedThisFrame = true;
}

void Mob::updateStuckResponse(float dt) {
    if (m_jumpCooldown > 0.0f) m_jumpCooldown -= dt;

    if (m_moveSpeed > 0.1f && m_blockedThisFrame) {
        m_blockedTimer += dt;

        // Nudge: hop once to clear a knee-high obstruction the step-up could not
        // resolve, then give up on this direction and head somewhere new.
        if (m_blockedTimer >= kBlockedJump) {
            tryJump();
        }
        if (m_blockedTimer >= kBlockedReroll) {
            rerollHeading();
            m_blockedTimer = 0.0f;
        }
    } else {
        // Decay faster than it accumulates so a single graze never triggers it.
        m_blockedTimer = std::max(0.0f, m_blockedTimer - dt * 2.0f);
    }
    m_blockedThisFrame = false;
}

void Mob::updateAI(float dt, const World& world, const glm::vec3& playerPos) {
    m_stateTimer -= dt;
    updateForwardVector();

    const float distToPlayer = glm::distance(m_position, playerPos);

    // Look at player if within 6 blocks during idle
    if ((m_goals & AiLookAt) && m_state == MobState::Idle && distToPlayer < 6.0f) {
        const glm::vec3 toPlayer = playerPos - m_position;
        const float targetLookYaw = glm::degrees(std::atan2(toPlayer.x, toPlayer.z));
        const float diff = wrapAngle(targetLookYaw - m_yaw);
        m_headYaw = std::clamp(diff, -50.0f, 50.0f);
        m_headPitch = std::clamp(-glm::degrees(std::atan2(toPlayer.y, glm::length(glm::vec2(toPlayer.x, toPlayer.z)))), -30.0f, 30.0f);
    } else {
        m_headYaw = glm::mix(m_headYaw, 0.0f, std::min(1.0f, dt * 5.0f));
        m_headPitch = glm::mix(m_headPitch, 0.0f, std::min(1.0f, dt * 5.0f));
    }

    // Water is sampled at the feet and at head height so a mob wading chest-deep
    // is treated as swimming rather than walking on the sea bed.
    const int bx = static_cast<int>(std::floor(m_position.x));
    const int bz = static_cast<int>(std::floor(m_position.z));
    const int feetY = static_cast<int>(std::floor(m_position.y));
    const int headY = static_cast<int>(std::floor(m_position.y + halfHeight() * 2.0f - 0.1f));
    m_inWater = isLiquid(world.getBlock(bx, feetY, bz)) ||
                isLiquid(world.getBlock(bx, headY, bz));

    if (m_hurtTimer > 0.0f) {
        m_hurtTimer -= dt;
        if (m_hurtTimer < 0.0f) m_hurtTimer = 0.0f;
    }

    if (m_attackCooldown > 0.0f) {
        m_attackCooldown -= dt;
    }

    switch (m_state) {
        case MobState::Idle: {
            m_moveSpeed = 0.0f;
            if ((m_goals & AiIdle) && m_stateTimer <= 0.0f) {
                if (m_goals & AiWander) {
                    m_state = MobState::Wander;
                    m_stateTimer = randomFloat(2.5f, 5.0f);
                    if (m_hasHome && glm::distance(glm::vec2(m_position.x, m_position.z), glm::vec2(m_homeVillage.x, m_homeVillage.z)) > 24.0f) {
                        // Steer back towards home village center
                        const glm::vec3 toHome = m_homeVillage - m_position;
                        m_targetYaw = glm::degrees(std::atan2(toHome.x, toHome.z)) + randomFloat(-20.0f, 20.0f);
                    } else {
                        m_targetYaw = wrapAngle(m_yaw + randomFloat(-90.0f, 90.0f));
                    }
                } else {
                    m_stateTimer = randomFloat(1.5f, 4.0f);
                    m_targetYaw = wrapAngle(m_yaw + randomFloat(-45.0f, 45.0f));
                }
            }
            break;
        }
        case MobState::Wander: {
            m_moveSpeed = (m_type == MobType::PigmanVillager) ? 1.8f : ((m_type == MobType::Pig) ? 1.6f : 1.3f);

            // Ledge guard: a wandering mob will not step off a cliff
            if ((m_goals & AiAvoidEdge) && m_onGround && !m_inWater &&
                dropAhead(world, kEdgeLookahead) > kSafeDrop) {
                m_targetYaw = wrapAngle(m_yaw + randomFloat(140.0f, 220.0f));
                m_stateTimer = std::min(m_stateTimer, 0.6f);
            }

            // Village perimeter leash during wander
            if (m_hasHome && (m_goals & AiHomeLeash)) {
                const float distToHome = glm::distance(glm::vec2(m_position.x, m_position.z), glm::vec2(m_homeVillage.x, m_homeVillage.z));
                if (distToHome > 28.0f) {
                    const glm::vec3 toHome = m_homeVillage - m_position;
                    m_targetYaw = glm::degrees(std::atan2(toHome.x, toHome.z));
                }
            }

            if (m_stateTimer <= 0.0f) {
                m_state = MobState::Idle;
                m_stateTimer = randomFloat(2.0f, 5.0f);
                m_moveSpeed = 0.0f;
            }
            break;
        }
        case MobState::Panic: {
            if (m_hurtTimer > 0.0f) {
                // Flinch during knockback recoil
                m_moveSpeed = 0.0f;
            } else {
                m_moveSpeed = (m_type == MobType::Pig) ? 4.8f : 4.0f;
                // Run away from panic source
                const glm::vec3 away = m_position - m_panicSource;
                if (glm::length(away) > 1e-4f) {
                    m_targetYaw = glm::degrees(std::atan2(away.x, away.z)) + randomFloat(-25.0f, 25.0f);
                }
                if (m_stateTimer <= 0.0f) {
                    m_state = MobState::Idle;
                    m_stateTimer = randomFloat(2.0f, 4.0f);
                    m_moveSpeed = 0.0f;
                }
            }
            break;
        }
        case MobState::Hostile: {
            const glm::vec3 toPlayer = playerPos - m_position;
            const float distPlayer = std::hypot(toPlayer.x, toPlayer.z);
            const float distHome = m_hasHome ? std::hypot(m_position.x - m_homeVillage.x, m_position.z - m_homeVillage.z) : 0.0f;

            if (m_hurtTimer > 0.0f) {
                // In hitstun / knockback recoil: mob flinches, turns toward attacker, but does not sprint forward yet
                m_moveSpeed = 0.0f;
                if (distPlayer > 0.1f) {
                    m_targetYaw = glm::degrees(std::atan2(toPlayer.x, toPlayer.z));
                }
            } else {
                // Recovered from knockback recoil: sprint toward attacker
                m_moveSpeed = 4.5f;
                m_aggroTimer -= dt;

                // Lose interest when:
                // 1. Aggro interest timer expires (m_aggroTimer <= 0.0f)
                // 2. Player flees beyond pursuit radius (distPlayer > 22.0f)
                // 3. Chases beyond village defense boundary (distHome > 34.0f)
                if (m_aggroTimer <= 0.0f || distPlayer > 22.0f || (m_hasHome && distHome > 34.0f)) {
                    m_state = m_hasHome ? MobState::ReturnToVillage : MobState::Idle;
                    m_stateTimer = randomFloat(4.0f, 7.0f);
                    m_moveSpeed = 0.0f;
                    m_aggroTimer = 0.0f;
                } else {
                    if (distPlayer > 0.1f) {
                        m_targetYaw = glm::degrees(std::atan2(toPlayer.x, toPlayer.z));
                    }
                    if (distPlayer <= 1.5f && m_attackCooldown <= 0.0f) {
                        m_attackCooldown = 1.0f;
                        m_aggroTimer = 10.0f; // Refresh interest when landing a hit
                    }
                }
            }
            break;
        }
        case MobState::ReturnToVillage: {
            m_moveSpeed = 2.4f; // Calm walk back to home village
            const glm::vec3 toHome = m_homeVillage - m_position;
            const float distHome = std::hypot(toHome.x, toHome.z);

            if (distHome < 5.0f || m_stateTimer <= 0.0f) {
                m_state = MobState::Idle;
                m_stateTimer = randomFloat(2.0f, 4.0f);
                m_moveSpeed = 0.0f;
            } else {
                m_targetYaw = glm::degrees(std::atan2(toHome.x, toHome.z));
            }
            break;
        }
    }

    // Smoothly rotate yaw toward targetYaw
    const float yawDiff = wrapAngle(m_targetYaw - m_yaw);
    float rotSpeed = kWanderTurnRate;
    if (m_state == MobState::Hostile) rotSpeed = kHostileTurnRate;
    else if (m_state == MobState::Panic) rotSpeed = kPanicTurnRate;

    const float step = std::clamp(yawDiff, -rotSpeed * dt, rotSpeed * dt);
    m_yaw = wrapAngle(m_yaw + step);
    updateForwardVector();

    // Keep mob within world boundaries
    const float minBound = 2.0f;
    const float maxBoundX = static_cast<float>(config::WORLD_CHUNKS_X * config::CHUNK_SIZE_X) - 2.0f;
    const float maxBoundZ = static_cast<float>(config::WORLD_CHUNKS_Z * config::CHUNK_SIZE_Z) - 2.0f;
    if (m_position.x < minBound || m_position.x > maxBoundX ||
        m_position.z < minBound || m_position.z > maxBoundZ) {
        m_targetYaw = wrapAngle(m_yaw + 180.0f);
    }
}

void Mob::updatePhysics(float dt, const World& world) {
    if (m_knockbackTimer > 0.0f) {
        m_knockbackTimer -= dt;
        if (m_knockbackTimer < 0.0f) m_knockbackTimer = 0.0f;
        // Smoothly decay knockback velocity with friction rather than instantly overriding
        const float friction = m_onGround ? 7.0f : 2.0f;
        m_velocity.x = glm::mix(m_velocity.x, 0.0f, std::min(1.0f, dt * friction));
        m_velocity.z = glm::mix(m_velocity.z, 0.0f, std::min(1.0f, dt * friction));
    } else {
        // Horizontal gait. Swimming is slower and heavier than walking.
        const float speed = m_moveSpeed * (m_inWater ? 0.6f : 1.0f);
        if (speed > 0.01f) {
            m_velocity.x = m_fwdX * speed;
            m_velocity.z = m_fwdZ * speed;
            m_animTime += dt * speed * 3.5f;
        } else {
            m_velocity.x *= 0.6f;
            m_velocity.z *= 0.6f;
        }
    }

    if (m_inWater && (m_goals & AiFloat)) {
        // Buoyancy with a surface deadband: a mob that is already at the
        // waterline stops rising instead of bobbing up and down forever.
        const int headY = static_cast<int>(std::floor(m_position.y + halfHeight() * 2.0f - 0.1f));
        const bool submerged = isLiquid(world.getBlock(static_cast<int>(std::floor(m_position.x)), headY,
                                                        static_cast<int>(std::floor(m_position.z))));
        if (submerged) {
            m_velocity.y = 2.2f;
        } else {
            m_velocity.y *= 0.7f;
        }
    } else {
        m_velocity.y -= GRAVITY * dt;
        if (m_velocity.y < -35.0f) m_velocity.y = -35.0f;
    }

    // Vertical first: this resolves m_onGround before the horizontal moves, so
    // step-up and jump know whether the mob is actually standing on something.
    m_onGround = false;
    moveAxis(world, m_velocity.y * dt, 1);
    moveAxis(world, m_velocity.x * dt, 0);
    moveAxis(world, m_velocity.z * dt, 2);

    // A body can be resting exactly on a surface without a downward move ever
    // being tested, which would leave m_onGround false forever and disable
    // stepping. Confirm with an explicit probe.
    if (!m_onGround && m_velocity.y <= 0.0f) {
        glm::vec3 probe = m_position;
        probe.y -= 0.02f;
        if (collides(world, probe)) m_onGround = true;
    }
}

void Mob::update(float dt, const World& world, const glm::vec3& playerPos) {
    if (m_hurtTimer > 0.0f) {
        m_hurtTimer -= dt;
    }

    updateAI(dt, world, playerPos);
    updatePhysics(dt, world);
    updateStuckResponse(dt);
}

void Mob::appendGeometry(std::vector<Vertex>& vertices, const World& world) const {
    // Trilinear smooth ambient light sampling at mob center
    const float fx = m_position.x - 0.5f;
    const float fy = m_position.y + 0.5f;
    const float fz = m_position.z - 0.5f;
    const int x0 = static_cast<int>(std::floor(fx));
    const int y0 = static_cast<int>(std::floor(fy));
    const int z0 = static_cast<int>(std::floor(fz));
    const float tx = fx - static_cast<float>(x0);
    const float ty = fy - static_cast<float>(y0);
    const float tz = fz - static_cast<float>(z0);

    float sunSum = 0.0f;
    float torchSum = 0.0f;
    for (int dz = 0; dz <= 1; ++dz) {
        for (int dy = 0; dy <= 1; ++dy) {
            for (int dx = 0; dx <= 1; ++dx) {
                const float w = (dx ? tx : (1.0f - tx)) *
                                (dy ? ty : (1.0f - ty)) *
                                (dz ? tz : (1.0f - tz));
                sunSum += static_cast<float>(world.getSunLight(x0 + dx, y0 + dy, z0 + dz)) * w;
                torchSum += static_cast<float>(world.getBlockLight(x0 + dx, y0 + dy, z0 + dz)) * w;
            }
        }
    }

    const float light = sunSum / 15.0f;
    const float torchLight = torchSum / 15.0f;
    
    // Flash vibrant red on hurt
    const float ao = (m_hurtTimer > 0.0f) ? -1.0f : 1.0f;

    // Base transform: Position + Mob Yaw
    glm::mat4 rootMat = glm::translate(glm::mat4(1.0f), m_position);
    rootMat = glm::rotate(rootMat, glm::radians(m_yaw), glm::vec3(0, 1, 0));

    const float legSwing = (m_moveSpeed > 0.05f) ? std::sin(m_animTime) * 0.32f : 0.0f;

    if (m_type == MobType::Pig) {
        // --- PIG MODEL ---
        const TextureTile skin = TextureTile::PigSkin;
        const TextureTile face = TextureTile::PigFace;
        const TextureTile snoutTile = TextureTile::PigSnout;

        // 1. Body: 0.60w x 0.50h x 0.85l (spans X: [-0.30, 0.30], Y: [0.17, 0.67], Z: [-0.425, 0.425])
        addOrientedBox(vertices, {0.0f, 0.42f, 0.0f}, {0.60f, 0.50f, 0.85f}, rootMat,
                       skin, skin, skin, skin, light, torchLight, ao);

        // 2. Head with head yaw/pitch: 0.44w x 0.44h x 0.44l
        glm::mat4 headMat = glm::translate(rootMat, glm::vec3(0.0f, 0.55f, 0.48f));
        headMat = glm::rotate(headMat, glm::radians(m_headYaw), glm::vec3(0, 1, 0));
        headMat = glm::rotate(headMat, glm::radians(m_headPitch), glm::vec3(1, 0, 0));

        addOrientedBox(vertices, {0.0f, 0.0f, 0.0f}, {0.44f, 0.44f, 0.44f}, headMat,
                       skin, skin, skin, face, light, torchLight, ao);

        // 3. Snout: 0.22w x 0.16h x 0.08l
        addOrientedBox(vertices, {0.0f, -0.08f, 0.26f}, {0.22f, 0.16f, 0.08f}, headMat,
                       snoutTile, snoutTile, snoutTile, snoutTile, light, torchLight, ao);

        // 4. Legs (FL, FR, BL, BR): 0.16w x 0.32h x 0.16l
        // Legs are tucked underneath the body with clear margin from outer torso sides.
        const float legW = 0.16f;
        const float legH = 0.32f;
        const float legD = 0.16f;

        const struct { float x, z, swing; } legs[4] = {
            { -0.15f,  0.22f,  legSwing }, // Front Left
            {  0.15f,  0.22f, -legSwing }, // Front Right
            { -0.15f, -0.22f, -legSwing }, // Back Left
            {  0.15f, -0.22f,  legSwing }, // Back Right
        };

        for (const auto& leg : legs) {
            glm::mat4 legMat = glm::translate(rootMat, glm::vec3(leg.x, legH, leg.z));
            legMat = glm::rotate(legMat, leg.swing, glm::vec3(1, 0, 0));
            addOrientedBox(vertices, {0.0f, -legH * 0.5f, 0.0f}, {legW, legH, legD}, legMat,
                           skin, skin, skin, skin, light, torchLight, ao);
        }
    } else if (m_type == MobType::Cow) {
        // --- COW MODEL ---
        const TextureTile skin = TextureTile::CowSkin;
        const TextureTile face = TextureTile::CowFace;
        const TextureTile horns = TextureTile::CowHorns;

        // 1. Body: 0.68w x 0.65h x 1.05l (spans X: [-0.34, 0.34], Y: [0.395, 1.045], Z: [-0.525, 0.525])
        addOrientedBox(vertices, {0.0f, 0.72f, 0.0f}, {0.68f, 0.65f, 1.05f}, rootMat,
                       skin, skin, skin, skin, light, torchLight, ao);

        // 2. Udder detail: 0.20w x 0.14h x 0.24l (pinkish)
        addOrientedBox(vertices, {0.0f, 0.40f, -0.22f}, {0.20f, 0.14f, 0.24f}, rootMat,
                       TextureTile::PigSkin, TextureTile::PigSkin, TextureTile::PigSkin, TextureTile::PigSkin, light, torchLight, ao);

        // 3. Head: 0.46w x 0.46h x 0.46l
        glm::mat4 headMat = glm::translate(rootMat, glm::vec3(0.0f, 0.95f, 0.62f));
        headMat = glm::rotate(headMat, glm::radians(m_headYaw), glm::vec3(0, 1, 0));
        headMat = glm::rotate(headMat, glm::radians(m_headPitch), glm::vec3(1, 0, 0));

        addOrientedBox(vertices, {0.0f, 0.0f, 0.0f}, {0.46f, 0.46f, 0.46f}, headMat,
                       skin, skin, skin, face, light, torchLight, ao);

        // 4. Horns (Left & Right): 0.08w x 0.16h x 0.08l
        addOrientedBox(vertices, {-0.28f, 0.22f, 0.0f}, {0.08f, 0.16f, 0.08f}, headMat,
                       horns, horns, horns, horns, light, torchLight, ao);
        addOrientedBox(vertices, { 0.28f, 0.22f, 0.0f}, {0.08f, 0.16f, 0.08f}, headMat,
                       horns, horns, horns, horns, light, torchLight, ao);

        // 5. Legs (FL, FR, BL, BR): 0.18w x 0.58h x 0.18l
        // Legs are inset from torso boundary (X: ±0.20 vs ±0.34, Z: ±0.28 vs ±0.525) to prevent clipping.
        const float legW = 0.18f;
        const float legH = 0.58f;
        const float legD = 0.18f;

        const struct { float x, z, swing; } legs[4] = {
            { -0.20f,  0.28f,  legSwing },
            {  0.20f,  0.28f, -legSwing },
            { -0.20f, -0.28f, -legSwing },
            {  0.20f, -0.28f,  legSwing },
        };

        for (const auto& leg : legs) {
            glm::mat4 legMat = glm::translate(rootMat, glm::vec3(leg.x, legH, leg.z));
            legMat = glm::rotate(legMat, leg.swing, glm::vec3(1, 0, 0));
            addOrientedBox(vertices, {0.0f, -legH * 0.5f, 0.0f}, {legW, legH, legD}, legMat,
                           skin, skin, skin, skin, light, torchLight, ao);
        }
    } else if (m_type == MobType::PigmanVillager) {
        // --- PIGMAN VILLAGER MODEL (Humanoid Biped) ---
        const TextureTile skin = TextureTile::PigmanSkin;
        const TextureTile face = TextureTile::PigmanFace;
        const TextureTile snoutTile = TextureTile::PigSnout;
        const TextureTile torsoTile = TextureTile::PigmanTorso;
        const TextureTile hoofTile = TextureTile::PigmanHoof;

        const float bipedSwing = (m_moveSpeed > 0.05f) ? std::sin(m_animTime) * 0.45f : 0.0f;

        // 1. Torso: 0.48w x 0.72h x 0.24l (Center Y = 1.08, spans [0.72, 1.44])
        addOrientedBox(vertices, {0.0f, 1.08f, 0.0f}, {0.48f, 0.72f, 0.24f}, rootMat,
                       skin, torsoTile, skin, torsoTile, light, torchLight, ao);

        // 2. Head with head yaw/pitch: 0.48w x 0.48h x 0.48l (Neck at Y = 1.44, Center Y = 1.68)
        glm::mat4 headMat = glm::translate(rootMat, glm::vec3(0.0f, 1.44f, 0.0f));
        headMat = glm::rotate(headMat, glm::radians(m_headYaw), glm::vec3(0, 1, 0));
        headMat = glm::rotate(headMat, glm::radians(m_headPitch), glm::vec3(1, 0, 0));

        addOrientedBox(vertices, {0.0f, 0.24f, 0.0f}, {0.48f, 0.48f, 0.48f}, headMat,
                       skin, skin, skin, face, light, torchLight, ao);

        // 3. Snout: 0.24w x 0.16h x 0.08l on front of face
        addOrientedBox(vertices, {0.0f, 0.18f, 0.28f}, {0.24f, 0.16f, 0.08f}, headMat,
                       snoutTile, snoutTile, snoutTile, snoutTile, light, torchLight, ao);

        // 4. Arms (Left & Right): 0.24w x 0.72h x 0.24l (Shoulder pivot at Y = 1.44, X = ±0.36)
        const float armW = 0.24f;
        const float armH = 0.72f;
        const float armD = 0.24f;

        float leftArmRot = -bipedSwing;
        float rightArmRot = bipedSwing;
        if (m_state == MobState::Hostile) {
            // Aggro raise arms forward
            leftArmRot = -1.25f + std::sin(m_animTime * 2.0f) * 0.15f;
            rightArmRot = -1.25f - std::sin(m_animTime * 2.0f) * 0.15f;
        }

        // Left Arm (X: -0.36, Y: 1.44)
        glm::mat4 lArmMat = glm::translate(rootMat, glm::vec3(-0.36f, 1.44f, 0.0f));
        lArmMat = glm::rotate(lArmMat, leftArmRot, glm::vec3(1, 0, 0));
        addOrientedBox(vertices, {0.0f, -armH * 0.5f, 0.0f}, {armW, armH, armD}, lArmMat,
                       skin, skin, skin, skin, light, torchLight, ao);

        // Right Arm (X: 0.36, Y: 1.44)
        glm::mat4 rArmMat = glm::translate(rootMat, glm::vec3(0.36f, 1.44f, 0.0f));
        rArmMat = glm::rotate(rArmMat, rightArmRot, glm::vec3(1, 0, 0));
        addOrientedBox(vertices, {0.0f, -armH * 0.5f, 0.0f}, {armW, armH, armD}, rArmMat,
                       skin, skin, skin, skin, light, torchLight, ao);

        // 5. Legs (Left & Right): 0.24w x 0.72h x 0.24l (Hip pivot at Y = 0.72, X = ±0.12)
        const float legW = 0.24f;
        const float legH = 0.72f;
        const float legD = 0.24f;

        // Left Leg (X: -0.12, Y: 0.72)
        glm::mat4 lLegMat = glm::translate(rootMat, glm::vec3(-0.12f, legH, 0.0f));
        lLegMat = glm::rotate(lLegMat, bipedSwing, glm::vec3(1, 0, 0));
        addOrientedBox(vertices, {0.0f, -legH * 0.5f, 0.0f}, {legW, legH, legD}, lLegMat,
                       skin, hoofTile, hoofTile, hoofTile, light, torchLight, ao);

        // Right Leg (X: 0.12, Y: 0.72)
        glm::mat4 rLegMat = glm::translate(rootMat, glm::vec3(0.12f, legH, 0.0f));
        rLegMat = glm::rotate(rLegMat, -bipedSwing, glm::vec3(1, 0, 0));
        addOrientedBox(vertices, {0.0f, -legH * 0.5f, 0.0f}, {legW, legH, legD}, rLegMat,
                       skin, hoofTile, hoofTile, hoofTile, light, torchLight, ao);
    }
}

} // namespace vox

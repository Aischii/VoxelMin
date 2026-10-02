#include "entity/ItemEntity.hpp"
#include "core/Config.hpp"
#include "world/World.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace vox {
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

void addItemBox(std::vector<Vertex>& vertices,
                const glm::vec3& center,
                const glm::vec3& size,
                const glm::mat4& transform,
                TextureTile tileTop,
                TextureTile tileSide,
                TextureTile tileBottom,
                float light,
                float torchLight) {
    const glm::vec3 half = size * 0.5f;
    const glm::vec2 tSize = tileSizeUV();

    struct FaceDef {
        glm::vec3 normal;
        glm::vec3 c1, c2, c3, c4;
        TextureTile tile;
    };

    const FaceDef faces[6] = {
        // Top (+Y)
        { {0.0f, 1.0f, 0.0f},
          {-half.x,  half.y,  half.z}, { half.x,  half.y,  half.z},
          { half.x,  half.y, -half.z}, {-half.x,  half.y, -half.z},
          tileTop },
        // Bottom (-Y)
        { {0.0f, -1.0f, 0.0f},
          {-half.x, -half.y, -half.z}, { half.x, -half.y, -half.z},
          { half.x, -half.y,  half.z}, {-half.x, -half.y,  half.z},
          tileBottom },
        // Front (+Z)
        { {0.0f, 0.0f, 1.0f},
          {-half.x, -half.y,  half.z}, { half.x, -half.y,  half.z},
          { half.x,  half.y,  half.z}, {-half.x,  half.y,  half.z},
          tileSide },
        // Back (-Z)
        { {0.0f, 0.0f, -1.0f},
          { half.x, -half.y, -half.z}, {-half.x, -half.y, -half.z},
          {-half.x,  half.y, -half.z}, { half.x,  half.y, -half.z},
          tileSide },
        // Right (+X)
        { {1.0f, 0.0f, 0.0f},
          { half.x, -half.y,  half.z}, { half.x, -half.y, -half.z},
          { half.x,  half.y, -half.z}, { half.x,  half.y,  half.z},
          tileSide },
        // Left (-X)
        { {-1.0f, 0.0f, 0.0f},
          {-half.x, -half.y, -half.z}, {-half.x, -half.y,  half.z},
          {-half.x,  half.y,  half.z}, {-half.x,  half.y, -half.z},
          tileSide },
    };

    const glm::mat3 normMat = glm::transpose(glm::inverse(glm::mat3(transform)));

    for (const auto& f : faces) {
        const glm::vec3 worldNorm = glm::normalize(normMat * f.normal);
        const glm::vec3 p1 = glm::vec3(transform * glm::vec4(center + f.c1, 1.0f));
        const glm::vec3 p2 = glm::vec3(transform * glm::vec4(center + f.c2, 1.0f));
        const glm::vec3 p3 = glm::vec3(transform * glm::vec4(center + f.c3, 1.0f));
        const glm::vec3 p4 = glm::vec3(transform * glm::vec4(center + f.c4, 1.0f));

        const glm::vec2 tMin = tileMinUV(f.tile);
        const glm::vec2 u0v0 = tMin;
        const glm::vec2 u1v0 = {tMin.x + tSize.x, tMin.y};
        const glm::vec2 u1v1 = {tMin.x + tSize.x, tMin.y + tSize.y};
        const glm::vec2 u0v1 = {tMin.x, tMin.y + tSize.y};

        // Two triangles CCW
        vertices.push_back({p1, worldNorm, u0v0, tMin, tSize, 1.0f, light, torchLight});
        vertices.push_back({p2, worldNorm, u1v0, tMin, tSize, 1.0f, light, torchLight});
        vertices.push_back({p3, worldNorm, u1v1, tMin, tSize, 1.0f, light, torchLight});

        vertices.push_back({p1, worldNorm, u0v0, tMin, tSize, 1.0f, light, torchLight});
        vertices.push_back({p3, worldNorm, u1v1, tMin, tSize, 1.0f, light, torchLight});
        vertices.push_back({p4, worldNorm, u0v1, tMin, tSize, 1.0f, light, torchLight});
    }
}

void addItemSprite(std::vector<Vertex>& vertices,
                   const glm::vec3& center,
                   float size,
                   const glm::mat4& transform,
                   TextureTile tile,
                   float light,
                   float torchLight) {
    const glm::vec2 tMin = tileMinUV(tile);
    const glm::vec2 tSize = tileSizeUV();
    const float hs = size * 0.5f;
    const float thick = 0.015f;

    // Front Face (+Z)
    const glm::vec3 f0 = glm::vec3(transform * glm::vec4(center + glm::vec3(-hs, -hs,  thick), 1.0f));
    const glm::vec3 f1 = glm::vec3(transform * glm::vec4(center + glm::vec3( hs, -hs,  thick), 1.0f));
    const glm::vec3 f2 = glm::vec3(transform * glm::vec4(center + glm::vec3( hs,  hs,  thick), 1.0f));
    const glm::vec3 f3 = glm::vec3(transform * glm::vec4(center + glm::vec3(-hs,  hs,  thick), 1.0f));
    const glm::vec3 normF = glm::normalize(glm::mat3(transform) * glm::vec3(0, 0, 1));

    vertices.push_back({ f0, normF, {0.0f, 0.0f}, tMin, tSize, 1.0f, light, torchLight });
    vertices.push_back({ f1, normF, {1.0f, 0.0f}, tMin, tSize, 1.0f, light, torchLight });
    vertices.push_back({ f2, normF, {1.0f, 1.0f}, tMin, tSize, 1.0f, light, torchLight });

    vertices.push_back({ f0, normF, {0.0f, 0.0f}, tMin, tSize, 1.0f, light, torchLight });
    vertices.push_back({ f2, normF, {1.0f, 1.0f}, tMin, tSize, 1.0f, light, torchLight });
    vertices.push_back({ f3, normF, {0.0f, 1.0f}, tMin, tSize, 1.0f, light, torchLight });

    // Back Face (-Z)
    const glm::vec3 b0 = glm::vec3(transform * glm::vec4(center + glm::vec3(-hs, -hs, -thick), 1.0f));
    const glm::vec3 b1 = glm::vec3(transform * glm::vec4(center + glm::vec3( hs, -hs, -thick), 1.0f));
    const glm::vec3 b2 = glm::vec3(transform * glm::vec4(center + glm::vec3( hs,  hs, -thick), 1.0f));
    const glm::vec3 b3 = glm::vec3(transform * glm::vec4(center + glm::vec3(-hs,  hs, -thick), 1.0f));
    const glm::vec3 normB = glm::normalize(glm::mat3(transform) * glm::vec3(0, 0, -1));

    vertices.push_back({ b1, normB, {1.0f, 0.0f}, tMin, tSize, 1.0f, light, torchLight });
    vertices.push_back({ b0, normB, {0.0f, 0.0f}, tMin, tSize, 1.0f, light, torchLight });
    vertices.push_back({ b3, normB, {0.0f, 1.0f}, tMin, tSize, 1.0f, light, torchLight });

    vertices.push_back({ b1, normB, {1.0f, 0.0f}, tMin, tSize, 1.0f, light, torchLight });
    vertices.push_back({ b3, normB, {0.0f, 1.0f}, tMin, tSize, 1.0f, light, torchLight });
    vertices.push_back({ b2, normB, {1.0f, 1.0f}, tMin, tSize, 1.0f, light, torchLight });
}

} // namespace

ItemEntity::ItemEntity(BlockId id, const glm::vec3& position, int count, const glm::vec3& initialVelocity)
    : m_blockId(id), m_count(count), m_position(position), m_velocity(initialVelocity) {
    if (glm::length(initialVelocity) < 0.01f) {
        const float vx = ((std::rand() % 1000) / 500.0f - 1.0f) * 1.5f;
        const float vz = ((std::rand() % 1000) / 500.0f - 1.0f) * 1.5f;
        const float vy = 3.5f + (std::rand() % 1000) / 1000.0f * 1.5f;
        m_velocity = glm::vec3(vx, vy, vz);
    }
}

bool ItemEntity::isWithinPickupRange(const glm::vec3& playerPos) const {
    const glm::vec3 playerCenter = playerPos + glm::vec3(0.0f, 0.85f, 0.0f);
    return canPickup() && glm::distance(m_position, playerCenter) <= PICKUP_RADIUS;
}

bool ItemEntity::collides(const World& world, const glm::vec3& pos) const {
    const int minX = static_cast<int>(std::floor(pos.x - HALF_SIZE));
    const int maxX = static_cast<int>(std::floor(pos.x + HALF_SIZE));
    const int minY = static_cast<int>(std::floor(pos.y - HALF_SIZE));
    const int maxY = static_cast<int>(std::floor(pos.y + HALF_SIZE));
    const int minZ = static_cast<int>(std::floor(pos.z - HALF_SIZE));
    const int maxZ = static_cast<int>(std::floor(pos.z + HALF_SIZE));

    for (int y = minY; y <= maxY; ++y) {
        for (int z = minZ; z <= maxZ; ++z) {
            for (int x = minX; x <= maxX; ++x) {
                const BlockId b = world.getBlock(x, y, z);
                if (isSolid(b)) {
                    const BlockBounds bb = blockBounds(b);
                    const float bMinX = static_cast<float>(x) + bb.minOffset.x;
                    const float bMaxX = static_cast<float>(x) + bb.maxOffset.x;
                    const float bMinY = static_cast<float>(y) + bb.minOffset.y;
                    const float bMaxY = static_cast<float>(y) + bb.maxOffset.y;
                    const float bMinZ = static_cast<float>(z) + bb.minOffset.z;
                    const float bMaxZ = static_cast<float>(z) + bb.maxOffset.z;

                    if (pos.x + HALF_SIZE > bMinX && pos.x - HALF_SIZE < bMaxX &&
                        pos.y + HALF_SIZE > bMinY && pos.y - HALF_SIZE < bMaxY &&
                        pos.z + HALF_SIZE > bMinZ && pos.z - HALF_SIZE < bMaxZ) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void ItemEntity::moveAxis(const World& world, float delta, int axis) {
    if (std::abs(delta) < 1e-5f) return;

    if (axis == 0) { // X
        m_position.x += delta;
        if (collides(world, m_position)) {
            m_position.x -= delta;
            m_velocity.x = 0.0f;
        }
    } else if (axis == 1) { // Y
        m_position.y += delta;
        if (collides(world, m_position)) {
            m_position.y -= delta;
            if (delta < 0.0f) {
                m_onGround = true;
            }
            m_velocity.y = 0.0f;
        } else {
            if (delta < 0.0f) m_onGround = false;
        }
    } else if (axis == 2) { // Z
        m_position.z += delta;
        if (collides(world, m_position)) {
            m_position.z -= delta;
            m_velocity.z = 0.0f;
        }
    }
}

void ItemEntity::update(float dt, const World& world, const glm::vec3& playerPos) {
    m_age += dt;
    if (m_pickupDelay > 0.0f) {
        m_pickupDelay = std::max(0.0f, m_pickupDelay - dt);
    }

    if (!m_onGround) {
        m_velocity.y -= GRAVITY * dt;
        m_velocity.y = std::max(m_velocity.y, -22.0f);
    }

    // Magnetism towards player only when very close and pickup delay elapsed
    const glm::vec3 playerCenter = playerPos + glm::vec3(0.0f, 0.85f, 0.0f);
    const float dist = glm::distance(m_position, playerCenter);
    if (m_pickupDelay <= 0.0f && dist <= MAGNET_RADIUS) {
        const glm::vec3 dir = glm::normalize(playerCenter - m_position);
        // Gentle, slow glide towards player
        const float pullStrength = 2.4f * (1.0f - dist / MAGNET_RADIUS) + 1.2f;
        m_velocity = glm::mix(m_velocity, dir * pullStrength, std::min(1.0f, dt * 5.0f));
        m_onGround = false;
    } else {
        // Air resistance when not being pulled
        m_velocity.x *= std::max(0.0f, 1.0f - 3.5f * dt);
        m_velocity.z *= std::max(0.0f, 1.0f - 3.5f * dt);
    }

    // Ground friction
    if (m_onGround) {
        m_velocity.x *= std::max(0.0f, 1.0f - 14.0f * dt);
        m_velocity.z *= std::max(0.0f, 1.0f - 14.0f * dt);
    }

    // Axis-by-axis collision movement
    moveAxis(world, m_velocity.x * dt, 0);
    moveAxis(world, m_velocity.y * dt, 1);
    moveAxis(world, m_velocity.z * dt, 2);
}

void ItemEntity::appendGeometry(std::vector<Vertex>& vertices, const World& world) const {
    if (isAir(m_blockId)) return;

    // Flash/blink when nearing 5-minute despawn (last 15 seconds)
    if (m_age > 285.0f) {
        const int flash = static_cast<int>((m_age - 285.0f) * 6.0f);
        if (flash % 2 == 1) return;
    }

    const BlockDef& def = blockDef(m_blockId);

    // Mini cube size: 0.25x0.25x0.25
    const glm::vec3 size(0.25f);
    const float bob = std::sin(m_age * 3.5f) * 0.04f;
    const glm::vec3 renderPos = m_position + glm::vec3(0.0f, 0.125f + bob, 0.0f);

    // Rotation around Y axis
    const float yawRadians = m_age * 2.2f;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), renderPos);
    model = glm::rotate(model, yawRadians, glm::vec3(0.0f, 1.0f, 0.0f));

    // Trilinear smooth ambient light sampling at item center
    const float fx = m_position.x - 0.5f;
    const float fy = m_position.y + 0.15f;
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

    if (isSolid(m_blockId)) {
        addItemBox(vertices, glm::vec3(0.0f), size, model, def.top, def.side, def.bottom, light, torchLight);
    } else {
        addItemSprite(vertices, glm::vec3(0.0f), 0.32f, model, def.side, light, torchLight);
    }
}

} // namespace vox

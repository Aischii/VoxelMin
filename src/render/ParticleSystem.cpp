#include "render/ParticleSystem.hpp"
#include "render/Renderer.hpp"
#include "world/Block.hpp"
#include "world/World.hpp"

#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace vox {
namespace {

float randF(float minVal, float maxVal) {
    const float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    return minVal + r * (maxVal - minVal);
}

} // namespace

ParticleSystem::~ParticleSystem() {
    shutdown();
}

bool ParticleSystem::init() {
    if (!m_shader.loadFromFiles(Renderer::resolveAsset("assets/shaders/particle.vert"),
                                Renderer::resolveAsset("assets/shaders/particle.frag"))) {
        return false;
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ParticleVertex) * 6 * 1024, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                          reinterpret_cast<void*>(offsetof(ParticleVertex, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                          reinterpret_cast<void*>(offsetof(ParticleVertex, color)));
    glBindVertexArray(0);

    return true;
}

void ParticleSystem::shutdown() {
    m_shader.destroy();
    if (m_vbo) { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    m_particles.clear();
}

void ParticleSystem::clear() {
    m_particles.clear();
}

void ParticleSystem::spawnFlame(const glm::vec3& pos) {
    if (m_particles.size() >= 1024) return;
    Particle p;
    p.pos = pos + glm::vec3(randF(-0.035f, 0.035f), randF(0.0f, 0.04f), randF(-0.035f, 0.035f));
    p.vel = glm::vec3(randF(-0.04f, 0.04f), randF(0.40f, 0.75f), randF(-0.04f, 0.04f));
    p.color = glm::vec4(1.0f, randF(0.70f, 0.95f), 0.20f, 0.95f);
    p.size = randF(0.12f, 0.18f);
    p.maxLife = randF(0.30f, 0.55f);
    p.life = p.maxLife;
    p.gravity = 0.0f;
    p.type = ParticleType::Flame;
    m_particles.push_back(p);
}

void ParticleSystem::spawnFire(const glm::vec3& pos, int count) {
    if (m_particles.size() >= 1024) return;
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = pos + glm::vec3(randF(-0.15f, 0.15f), randF(0.0f, 0.15f), randF(-0.15f, 0.15f));
        p.vel = glm::vec3(randF(-0.10f, 0.10f), randF(0.55f, 1.10f), randF(-0.10f, 0.10f));
        p.color = glm::vec4(1.0f, randF(0.50f, 0.90f), 0.10f, 1.0f);
        p.size = randF(0.16f, 0.26f);
        p.maxLife = randF(0.40f, 0.75f);
        p.life = p.maxLife;
        p.gravity = 0.0f;
        p.type = ParticleType::Flame;
        m_particles.push_back(p);

        if (randF(0.0f, 1.0f) < 0.30f) {
            spawnSmoke(pos + glm::vec3(0.0f, 0.15f, 0.0f));
        }
    }
}

void ParticleSystem::spawnSmoke(const glm::vec3& pos) {
    if (m_particles.size() >= 1024) return;
    Particle p;
    p.pos = pos + glm::vec3(randF(-0.03f, 0.03f), randF(0.04f, 0.08f), randF(-0.03f, 0.03f));
    p.vel = glm::vec3(randF(-0.05f, 0.05f), randF(0.35f, 0.65f), randF(-0.05f, 0.05f));
    const float grey = randF(0.18f, 0.32f);
    p.color = glm::vec4(grey, grey, grey, 0.65f);
    p.size = randF(0.10f, 0.16f);
    p.maxLife = randF(0.50f, 0.95f);
    p.life = p.maxLife;
    p.gravity = 0.0f;
    p.type = ParticleType::Smoke;
    m_particles.push_back(p);
}

void ParticleSystem::spawnFallingLeaf(const glm::vec3& pos, const glm::vec3& color) {
    if (m_particles.size() >= 1024) return;
    Particle p;
    p.pos = pos + glm::vec3(randF(-0.35f, 0.35f), randF(-0.1f, 0.0f), randF(-0.35f, 0.35f));
    p.vel = glm::vec3(randF(-0.1f, 0.2f), -0.55f, randF(-0.1f, 0.1f));
    const float cVar = randF(0.88f, 1.12f);
    p.color = glm::vec4(color.r * cVar, color.g * cVar, color.b * cVar, 0.95f);
    p.size = randF(0.09f, 0.14f);
    p.maxLife = randF(4.0f, 6.5f);
    p.life = p.maxLife;
    p.gravity = 0.0f;
    p.type = ParticleType::Leaf;
    p.swayPhase = randF(0.0f, 6.28f);
    p.swaySpeed = randF(2.0f, 3.2f);
    p.resting = false;
    m_particles.push_back(p);
}

void ParticleSystem::spawnHitParticles(const glm::vec3& pos, const glm::vec3& hitDir, bool isCrit, int count) {
    if (m_particles.size() >= 1024) return;
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = pos + glm::vec3(randF(-0.15f, 0.15f), randF(-0.15f, 0.15f), randF(-0.15f, 0.15f));
        const glm::vec3 spread(randF(-0.6f, 0.6f), randF(-0.3f, 0.6f), randF(-0.6f, 0.6f));
        const float speed = isCrit ? randF(3.5f, 6.5f) : randF(2.0f, 4.2f);
        p.vel = glm::normalize(hitDir + spread) * speed;
        if (isCrit) {
            p.color = glm::vec4(1.0f, randF(0.85f, 1.0f), randF(0.2f, 0.4f), 1.0f); // Bright Gold/White spark
            p.size = randF(0.08f, 0.14f);
            p.maxLife = randF(0.25f, 0.45f);
        } else {
            p.color = glm::vec4(1.0f, randF(0.92f, 1.0f), randF(0.80f, 0.95f), 1.0f); // White slash flash
            p.size = randF(0.06f, 0.10f);
            p.maxLife = randF(0.18f, 0.32f);
        }
        p.life = p.maxLife;
        p.gravity = 4.0f;
        p.type = ParticleType::Spark;
        p.resting = false;
        m_particles.push_back(p);
    }
}

void ParticleSystem::spawnBloodSplatter(const glm::vec3& pos, const glm::vec3& hitDir, int count) {
    if (m_particles.size() >= 1024) return;
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = pos + glm::vec3(randF(-0.12f, 0.12f), randF(-0.12f, 0.12f), randF(-0.12f, 0.12f));
        const glm::vec3 sprayDir = hitDir + glm::vec3(randF(-0.55f, 0.55f), randF(-0.2f, 0.7f), randF(-0.55f, 0.55f));
        const float speed = randF(1.5f, 4.0f);
        p.vel = glm::normalize(sprayDir) * speed + glm::vec3(0.0f, randF(0.5f, 1.8f), 0.0f);
        const float redShade = randF(0.65f, 0.95f);
        p.color = glm::vec4(redShade, randF(0.04f, 0.12f), randF(0.06f, 0.14f), 0.95f); // Crimson blood
        p.size = randF(0.06f, 0.12f);
        p.maxLife = randF(0.60f, 1.8f);
        p.life = p.maxLife;
        p.gravity = 18.0f;
        p.type = ParticleType::Blood;
        p.resting = false;
        m_particles.push_back(p);
    }
}

void ParticleSystem::spawnDigParticles(const glm::vec3& blockPos, const glm::ivec3& normal, uint8_t blockId,
                                       const World& world, float sunlight, int count) {
    if (m_particles.size() >= 1024) return;
    const glm::vec3 baseCol = blockColor(static_cast<BlockId>(blockId));
    const glm::vec3 n(normal);

    const int bx = static_cast<int>(std::floor(blockPos.x + n.x * 0.5f));
    const int by = static_cast<int>(std::floor(blockPos.y + n.y * 0.5f));
    const int bz = static_cast<int>(std::floor(blockPos.z + n.z * 0.5f));
    const float sunLight = static_cast<float>(world.getSunLight(bx, by, bz)) / 15.0f;
    const float blockLight = static_cast<float>(world.getBlockLight(bx, by, bz)) / 15.0f;
    const float light = std::clamp(sunLight * sunlight + blockLight * (1.0f - sunLight * sunlight * 0.5f), 0.08f, 1.0f);

    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = blockPos + glm::vec3(0.5f) + n * 0.52f +
                glm::vec3(randF(-0.35f, 0.35f), randF(-0.35f, 0.35f), randF(-0.35f, 0.35f));
        p.vel = n * randF(1.2f, 2.5f) +
                glm::vec3(randF(-1.5f, 1.5f), randF(0.8f, 2.2f), randF(-1.5f, 1.5f));
        const float cVar = randF(0.85f, 1.15f) * light;
        p.color = glm::vec4(baseCol.r * cVar, baseCol.g * cVar, baseCol.b * cVar, 1.0f);
        p.size = randF(0.06f, 0.12f);
        p.maxLife = randF(0.30f, 0.55f);
        p.life = p.maxLife;
        p.gravity = 14.0f;
        m_particles.push_back(p);
    }
}

void ParticleSystem::spawnBlockBreakParticles(const glm::vec3& blockPos, uint8_t blockId,
                                              const World& world, float sunlight, int count) {
    if (m_particles.size() >= 1024) return;
    const glm::vec3 baseCol = blockColor(static_cast<BlockId>(blockId));
    const glm::vec3 center = blockPos + glm::vec3(0.5f);

    const int bx = static_cast<int>(std::floor(blockPos.x));
    const int by = static_cast<int>(std::floor(blockPos.y));
    const int bz = static_cast<int>(std::floor(blockPos.z));
    const float sunLight = static_cast<float>(world.getSunLight(bx, by, bz)) / 15.0f;
    const float blockLight = static_cast<float>(world.getBlockLight(bx, by, bz)) / 15.0f;
    const float light = std::clamp(sunLight * sunlight + blockLight * (1.0f - sunLight * sunlight * 0.5f), 0.08f, 1.0f);

    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = center + glm::vec3(randF(-0.40f, 0.40f), randF(-0.40f, 0.40f), randF(-0.40f, 0.40f));
        p.vel = glm::vec3(randF(-2.5f, 2.5f), randF(1.5f, 4.2f), randF(-2.5f, 2.5f));
        const float cVar = randF(0.80f, 1.20f) * light;
        p.color = glm::vec4(baseCol.r * cVar, baseCol.g * cVar, baseCol.b * cVar, 1.0f);
        p.size = randF(0.08f, 0.16f);
        p.maxLife = randF(0.45f, 0.85f);
        p.life = p.maxLife;
        p.gravity = 14.0f;
        m_particles.push_back(p);
    }
}

void ParticleSystem::update(float dt, const World& world, const glm::vec3& playerPos) {
    m_spawnTimer += dt;
    m_leafScanTimer += dt;

    const int px = static_cast<int>(std::floor(playerPos.x));
    const int py = static_cast<int>(std::floor(playerPos.y));
    const int pz = static_cast<int>(std::floor(playerPos.z));

    // 1. Scan nearby blocks around player for torches to emit flame & smoke
    if (m_spawnTimer >= 0.08f) {
        m_spawnTimer = 0.0f;
        const int radius = 12;

        for (int i = 0; i < 48; ++i) {
            const int wx = px + (std::rand() % (2 * radius + 1) - radius);
            const int wz = pz + (std::rand() % (2 * radius + 1) - radius);
            const int wy = py + (std::rand() % 13 - 6);
            if (wy < 0 || wy >= Chunk::H) continue;
            const BlockId blk = world.getBlock(wx, wy, wz);
            if (isTorch(blk)) {
                glm::vec3 torchTop(static_cast<float>(wx) + 0.5f,
                                   static_cast<float>(wy) + 0.62f,
                                   static_cast<float>(wz) + 0.5f);
                if (blk == BlockId::TorchWallWest) {
                    torchTop = glm::vec3(static_cast<float>(wx) + 0.32f,
                                         static_cast<float>(wy) + 0.72f,
                                         static_cast<float>(wz) + 0.5f);
                } else if (blk == BlockId::TorchWallEast) {
                    torchTop = glm::vec3(static_cast<float>(wx) + 0.68f,
                                         static_cast<float>(wy) + 0.72f,
                                         static_cast<float>(wz) + 0.5f);
                } else if (blk == BlockId::TorchWallNorth) {
                    torchTop = glm::vec3(static_cast<float>(wx) + 0.5f,
                                         static_cast<float>(wy) + 0.72f,
                                         static_cast<float>(wz) + 0.32f);
                } else if (blk == BlockId::TorchWallSouth) {
                    torchTop = glm::vec3(static_cast<float>(wx) + 0.5f,
                                         static_cast<float>(wy) + 0.72f,
                                         static_cast<float>(wz) + 0.68f);
                }

                spawnFlame(torchTop);
                if (randF(0.0f, 1.0f) < 0.40f) {
                    spawnSmoke(torchTop);
                }
            }
        }
    }

    // 2. Falling Leaves spawner (Fourmisain-inspired canopy particles)
    if (m_leafScanTimer >= 0.08f) {
        m_leafScanTimer = 0.0f;
        const int leafRadius = 18;

        for (int i = 0; i < 16; ++i) {
            const int rx = px + (std::rand() % (2 * leafRadius + 1) - leafRadius);
            const int rz = pz + (std::rand() % (2 * leafRadius + 1) - leafRadius);
            const int ry = py + (std::rand() % 17 - 4);

            if (ry > 1 && ry < Chunk::H - 1) {
                const BlockId blk = world.getBlock(rx, ry, rz);
                if (blk == BlockId::Leaves) {
                    const BlockId below = world.getBlock(rx, ry - 1, rz);
                    if (below == BlockId::Air) {
                        const glm::vec3 leafPos(static_cast<float>(rx) + 0.5f,
                                                static_cast<float>(ry) - 0.05f,
                                                static_cast<float>(rz) + 0.5f);
                        spawnFallingLeaf(leafPos);
                    }
                }
            }
        }
    }

    // 3. Update active particles & physics
    for (auto it = m_particles.begin(); it != m_particles.end(); ) {
        it->life -= dt;
        if (it->life <= 0.0f) {
            it = m_particles.erase(it);
        } else {
            if (it->resting) {
                // Fade out softly while resting on surface
                const float progress = it->life / it->maxLife;
                it->color.a = progress * 0.85f;
            } else if (it->type == ParticleType::Leaf) {
                it->swayPhase += dt * it->swaySpeed;
                it->vel.x = std::sin(it->swayPhase) * 0.45f + 0.12f;
                it->vel.z = std::cos(it->swayPhase * 0.85f) * 0.35f;
                it->vel.y = -0.55f;
                it->pos += it->vel * dt;

                const int bx = static_cast<int>(std::floor(it->pos.x));
                const int by = static_cast<int>(std::floor(it->pos.y));
                const int bz = static_cast<int>(std::floor(it->pos.z));
                const BlockId groundBlk = world.getBlock(bx, by, bz);
                if (isSolid(groundBlk) || groundBlk == BlockId::Water) {
                    it->resting = true;
                    it->vel = glm::vec3(0.0f);
                    it->pos.y = static_cast<float>(by + 1) + 0.02f;
                    it->life = std::min(it->life, 1.8f);
                }
                const float progress = it->life / it->maxLife;
                it->color.a = progress * 0.95f;
            } else if (it->type == ParticleType::Flame) {
                it->vel.y += 0.35f * dt; // upward thermal lift
                it->vel.x += randF(-0.1f, 0.1f) * dt;
                it->vel.z += randF(-0.1f, 0.1f) * dt;
                it->pos += it->vel * dt;
                const float progress = it->life / it->maxLife;
                it->size = (0.04f + 0.14f * progress); // shrink as it burns
                it->color.r = 1.0f;
                it->color.g = std::clamp(progress * 0.95f, 0.20f, 0.95f);
                it->color.b = std::clamp(progress * 0.35f - 0.05f, 0.0f, 0.35f);
                it->color.a = progress * 0.95f;
            } else if (it->type == ParticleType::Smoke) {
                it->vel.y += 0.15f * dt;
                it->pos += it->vel * dt;
                const float progress = it->life / it->maxLife;
                it->size = (0.16f - 0.06f * progress); // expand as smoke dissipates
                it->color.a = progress * 0.55f;
            } else if (it->type == ParticleType::Blood) {
                it->vel.y -= it->gravity * dt;
                it->vel.x *= std::max(0.0f, 1.0f - dt * 2.2f);
                it->vel.z *= std::max(0.0f, 1.0f - dt * 2.2f);
                it->pos += it->vel * dt;

                const int bx = static_cast<int>(std::floor(it->pos.x));
                const int by = static_cast<int>(std::floor(it->pos.y));
                const int bz = static_cast<int>(std::floor(it->pos.z));
                const BlockId groundBlk = world.getBlock(bx, by, bz);
                if (isSolid(groundBlk)) {
                    it->resting = true;
                    it->vel = glm::vec3(0.0f);
                    it->pos.y = static_cast<float>(by + 1) + 0.015f;
                    it->life = std::min(it->life, 2.2f);
                }
                const float progress = it->life / it->maxLife;
                it->color.a = progress * 0.95f;
            } else {
                it->vel.y -= it->gravity * dt;
                it->pos += it->vel * dt;
                const float progress = it->life / it->maxLife;
                it->color.a = progress * 0.95f;
            }
            ++it;
        }
    }
}

void ParticleSystem::render(const Camera& camera, float aspect) {
    if (m_particles.empty()) return;

    static std::vector<ParticleVertex> vertices;
    vertices.clear();
    vertices.reserve(m_particles.size() * 6);

    const glm::vec3 right = camera.right();
    const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    for (const auto& p : m_particles) {
        const float hs = p.size * 0.5f;
        const glm::vec3 r = right * hs;
        const glm::vec3 u = up * hs;

        const glm::vec3 p0 = p.pos - r - u;
        const glm::vec3 p1 = p.pos + r - u;
        const glm::vec3 p2 = p.pos + r + u;
        const glm::vec3 p3 = p.pos - r + u;

        vertices.push_back({ p0, p.color });
        vertices.push_back({ p1, p.color });
        vertices.push_back({ p2, p.color });

        vertices.push_back({ p0, p.color });
        vertices.push_back({ p2, p.color });
        vertices.push_back({ p3, p.color });
    }

    if (vertices.empty()) return;

    const glm::mat4 viewProjection = camera.projectionMatrix(aspect) * camera.viewMatrix();

    m_shader.use();
    m_shader.setMat4("uVP", viewProjection);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // Don't write depth for soft blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive/soft flame blend
    glDisable(GL_CULL_FACE);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(ParticleVertex)),
                 vertices.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
}

} // namespace vox

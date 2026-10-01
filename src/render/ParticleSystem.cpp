#include "render/ParticleSystem.hpp"
#include "render/Renderer.hpp"
#include "world/Block.hpp"
#include "world/World.hpp"

#include <GL/glew.h>
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
    p.pos = pos + glm::vec3(randF(-0.04f, 0.04f), randF(0.0f, 0.05f), randF(-0.04f, 0.04f));
    p.vel = glm::vec3(randF(-0.05f, 0.05f), randF(0.40f, 0.75f), randF(-0.05f, 0.05f));
    p.color = glm::vec4(1.0f, randF(0.60f, 0.92f), 0.12f, 0.95f);
    p.size = randF(0.14f, 0.22f);
    p.maxLife = randF(0.35f, 0.65f);
    p.life = p.maxLife;
    p.gravity = 0.0f;
    m_particles.push_back(p);
}

void ParticleSystem::spawnSmoke(const glm::vec3& pos) {
    if (m_particles.size() >= 1024) return;
    Particle p;
    p.pos = pos + glm::vec3(randF(-0.03f, 0.03f), randF(0.04f, 0.08f), randF(-0.03f, 0.03f));
    p.vel = glm::vec3(randF(-0.06f, 0.06f), randF(0.45f, 0.80f), randF(-0.06f, 0.06f));
    const float grey = randF(0.15f, 0.30f);
    p.color = glm::vec4(grey, grey, grey, 0.65f);
    p.size = randF(0.12f, 0.18f);
    p.maxLife = randF(0.60f, 1.1f);
    p.life = p.maxLife;
    p.gravity = 0.0f;
    m_particles.push_back(p);
}

void ParticleSystem::spawnDigParticles(const glm::vec3& blockPos, const glm::ivec3& normal, uint8_t blockId, int count) {
    if (m_particles.size() >= 1024) return;
    const glm::vec3 baseCol = blockColor(static_cast<BlockId>(blockId));
    const glm::vec3 n(normal);

    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = blockPos + glm::vec3(0.5f) + n * 0.52f +
                glm::vec3(randF(-0.35f, 0.35f), randF(-0.35f, 0.35f), randF(-0.35f, 0.35f));
        p.vel = n * randF(1.2f, 2.5f) +
                glm::vec3(randF(-1.5f, 1.5f), randF(0.8f, 2.2f), randF(-1.5f, 1.5f));
        const float cVar = randF(0.85f, 1.15f);
        p.color = glm::vec4(baseCol.r * cVar, baseCol.g * cVar, baseCol.b * cVar, 1.0f);
        p.size = randF(0.06f, 0.12f);
        p.maxLife = randF(0.30f, 0.55f);
        p.life = p.maxLife;
        p.gravity = 14.0f;
        m_particles.push_back(p);
    }
}

void ParticleSystem::spawnBlockBreakParticles(const glm::vec3& blockPos, uint8_t blockId, int count) {
    if (m_particles.size() >= 1024) return;
    const glm::vec3 baseCol = blockColor(static_cast<BlockId>(blockId));
    const glm::vec3 center = blockPos + glm::vec3(0.5f);

    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = center + glm::vec3(randF(-0.40f, 0.40f), randF(-0.40f, 0.40f), randF(-0.40f, 0.40f));
        p.vel = glm::vec3(randF(-2.5f, 2.5f), randF(1.5f, 4.2f), randF(-2.5f, 2.5f));
        const float cVar = randF(0.80f, 1.20f);
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

    // Scan nearby blocks around player for torches to emit flame & smoke
    if (m_spawnTimer >= 0.04f) {
        m_spawnTimer = 0.0f;

        const int px = static_cast<int>(std::floor(playerPos.x));
        const int py = static_cast<int>(std::floor(playerPos.y));
        const int pz = static_cast<int>(std::floor(playerPos.z));
        const int radius = 14;

        for (int dz = -radius; dz <= radius; ++dz) {
            for (int dx = -radius; dx <= radius; ++dx) {
                for (int dy = -8; dy <= 8; ++dy) {
                    const int wx = px + dx;
                    const int wy = py + dy;
                    const int wz = pz + dz;
                    const BlockId blk = world.getBlock(wx, wy, wz);
                    if (isTorch(blk)) {
                        if (randF(0.0f, 1.0f) < 0.60f) {
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
            }
        }
    }

    // Update particles
    for (auto it = m_particles.begin(); it != m_particles.end(); ) {
        it->life -= dt;
        if (it->life <= 0.0f) {
            it = m_particles.erase(it);
        } else {
            it->vel.y -= it->gravity * dt;
            it->pos += it->vel * dt;
            const float progress = it->life / it->maxLife;
            it->color.a = progress * 0.95f;
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

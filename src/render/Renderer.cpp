#include "render/Renderer.hpp"
#include "core/Config.hpp"
#include "core/Log.hpp"
#include "entity/EntityManager.hpp"
#include "player/Player.hpp"
#include "render/Camera.hpp"
#include "render/Frustum.hpp"
#include "world/Chunk.hpp"
#include "world/World.hpp"

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <random>
#include <vector>

namespace vox {

std::string Renderer::resolveAsset(const std::string& relativePath) {
    const std::string candidates[] = {relativePath, "../" + relativePath, "../../" + relativePath};
    for (const std::string& candidate : candidates) {
        std::ifstream file(candidate);
        if (file.good()) return candidate;
    }
    return relativePath;
}

bool Renderer::init() {
    if (!m_chunkShader.loadFromFiles(resolveAsset("assets/shaders/chunk.vert"),
                                     resolveAsset("assets/shaders/chunk.frag"))) {
        return false;
    }
    if (!m_lineShader.loadFromFiles(resolveAsset("assets/shaders/line.vert"),
                                    resolveAsset("assets/shaders/line.frag"))) {
        return false;
    }
    if (!m_uiShader.loadFromFiles(resolveAsset("assets/shaders/ui.vert"),
                                  resolveAsset("assets/shaders/ui.frag"))) {
        return false;
    }
    if (!m_textShader.loadFromFiles(resolveAsset("assets/shaders/text.vert"),
                                    resolveAsset("assets/shaders/text.frag"))) {
        return false;
    }
    if (!m_spriteShader.loadFromFiles(resolveAsset("assets/shaders/sprite.vert"),
                                      resolveAsset("assets/shaders/sprite.frag"))) {
        return false;
    }
    if (!m_underwaterShader.loadFromFiles(resolveAsset("assets/shaders/underwater.vert"),
                                          resolveAsset("assets/shaders/underwater.frag"))) {
        return false;
    }
    if (!m_skyShader.loadFromFiles(resolveAsset("assets/shaders/sky.vert"),
                                   resolveAsset("assets/shaders/sky.frag"))) {
        return false;
    }

    m_atlas.createAtlas();
    if (!m_font.build()) return false;
    if (!m_particles.init()) return false;
    initSky();

    // --- Block selection outline: a slightly enlarged unit-cube wireframe. ---
    const float lo = -0.002f;
    const float hi = 1.002f;
    const float corners[8][3] = {
        {lo, lo, lo}, {hi, lo, lo}, {hi, lo, hi}, {lo, lo, hi},
        {lo, hi, lo}, {hi, hi, lo}, {hi, hi, hi}, {lo, hi, hi},
    };
    const int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7},
    };
    float lineData[24 * 3];
    for (int e = 0; e < 12; ++e) {
        for (int v = 0; v < 2; ++v) {
            for (int c = 0; c < 3; ++c) {
                lineData[(e * 2 + v) * 3 + c] = corners[edges[e][v]][c];
            }
        }
    }

    glGenVertexArrays(1, &m_lineVao);
    glGenBuffers(1, &m_lineVbo);
    glBindVertexArray(m_lineVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineData), lineData, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glBindVertexArray(0);

    // --- UI quad buffer: position (vec2) + colour (vec4). -------------------
    glGenVertexArrays(1, &m_uiVao);
    glGenBuffers(1, &m_uiVbo);
    glBindVertexArray(m_uiVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_uiVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(UIVertex) * 8, nullptr, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                          reinterpret_cast<void*>(offsetof(UIVertex, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                          reinterpret_cast<void*>(offsetof(UIVertex, color)));
    glBindVertexArray(0);

    // --- Text buffer: position (vec2) + uv (vec2). --------------------------
    glGenVertexArrays(1, &m_textVao);
    glGenBuffers(1, &m_textVbo);
    glBindVertexArray(m_textVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_textVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(TextVertex) * 1024, nullptr, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex),
                          reinterpret_cast<void*>(offsetof(TextVertex, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex),
                          reinterpret_cast<void*>(offsetof(TextVertex, uv)));
    glBindVertexArray(0);

    // --- Sprite buffer: position (vec2) + uv (vec2). ------------------------
    glGenVertexArrays(1, &m_spriteVao);
    glGenBuffers(1, &m_spriteVbo);
    glBindVertexArray(m_spriteVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_spriteVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SpriteVertex) * 6, nullptr, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex),
                          reinterpret_cast<void*>(offsetof(SpriteVertex, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex),
                          reinterpret_cast<void*>(offsetof(SpriteVertex, uv)));
    glBindVertexArray(0);

    // --- Entity buffer: Vertex layout matching chunk shader -----------------
    glGenVertexArrays(1, &m_entityVao);
    glGenBuffers(1, &m_entityVbo);
    glBindVertexArray(m_entityVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_entityVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 4096, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, uv)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, tileMin)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, tileSize)));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, ao)));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, light)));
    glBindVertexArray(0);

    // --- Underwater fullscreen quad buffer: pos (vec2) + uv (vec2) ---------
    const float underwaterQuad[6 * 4] = {
        // pos.x, pos.y, uv.x, uv.y
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
    };
    glGenVertexArrays(1, &m_underwaterVao);
    glGenBuffers(1, &m_underwaterVbo);
    glBindVertexArray(m_underwaterVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_underwaterVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(underwaterQuad), underwaterQuad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);

    log::info("Renderer initialised (OpenGL %s)", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    return true;
}

void Renderer::shutdown() {
    m_chunkShader.destroy();
    m_skyShader.destroy();
    m_lineShader.destroy();
    m_uiShader.destroy();
    m_textShader.destroy();
    m_spriteShader.destroy();
    m_underwaterShader.destroy();
    m_atlas.destroy();
    m_font.destroy();
    m_particles.shutdown();

    if (m_lineVbo)       { glDeleteBuffers(1, &m_lineVbo); m_lineVbo = 0; }
    if (m_lineVao)       { glDeleteVertexArrays(1, &m_lineVao); m_lineVao = 0; }
    if (m_uiVbo)         { glDeleteBuffers(1, &m_uiVbo); m_uiVbo = 0; }
    if (m_uiVao)         { glDeleteVertexArrays(1, &m_uiVao); m_uiVao = 0; }
    if (m_textVbo)       { glDeleteBuffers(1, &m_textVbo); m_textVbo = 0; }
    if (m_textVao)       { glDeleteVertexArrays(1, &m_textVao); m_textVao = 0; }
    if (m_spriteVbo)     { glDeleteBuffers(1, &m_spriteVbo); m_spriteVbo = 0; }
    if (m_spriteVao)     { glDeleteVertexArrays(1, &m_spriteVao); m_spriteVao = 0; }
    if (m_entityVbo)     { glDeleteBuffers(1, &m_entityVbo); m_entityVbo = 0; }
    if (m_entityVao)     { glDeleteVertexArrays(1, &m_entityVao); m_entityVao = 0; }
    if (m_underwaterVbo) { glDeleteBuffers(1, &m_underwaterVbo); m_underwaterVbo = 0; }
    if (m_underwaterVao) { glDeleteVertexArrays(1, &m_underwaterVao); m_underwaterVao = 0; }
    if (m_skyVbo)        { glDeleteBuffers(1, &m_skyVbo); m_skyVbo = 0; }
    if (m_skyVao)        { glDeleteVertexArrays(1, &m_skyVao); m_skyVao = 0; }
    if (m_starVbo)       { glDeleteBuffers(1, &m_starVbo); m_starVbo = 0; }
    if (m_starVao)       { glDeleteVertexArrays(1, &m_starVao); m_starVao = 0; }
}

void Renderer::initSky() {
    glGenVertexArrays(1, &m_skyVao);
    glGenBuffers(1, &m_skyVbo);
    glBindVertexArray(m_skyVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_skyVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SkyVertex) * 64, nullptr, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkyVertex),
                          reinterpret_cast<void*>(offsetof(SkyVertex, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(SkyVertex),
                          reinterpret_cast<void*>(offsetof(SkyVertex, color)));
    glBindVertexArray(0);

    std::vector<SkyVertex> stars;
    stars.reserve(450);
    std::mt19937 rng(54321);
    std::uniform_real_distribution<float> distTheta(0.0f, glm::two_pi<float>());
    std::uniform_real_distribution<float> distPhi(0.05f, glm::half_pi<float>() * 0.95f);
    std::uniform_real_distribution<float> distBrightness(0.60f, 1.0f);

    for (int i = 0; i < 450; ++i) {
        const float theta = distTheta(rng);
        const float phi = distPhi(rng);
        const float b = distBrightness(rng);
        const float radius = 95.0f;

        const float x = radius * std::sin(phi) * std::cos(theta);
        const float y = radius * std::cos(phi);
        const float z = radius * std::sin(phi) * std::sin(theta);

        stars.push_back({{x, y, z}, glm::vec4(b, b, b * 1.15f, 1.0f)});
    }

    m_starCount = stars.size();
    glGenVertexArrays(1, &m_starVao);
    glGenBuffers(1, &m_starVbo);
    glBindVertexArray(m_starVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_starVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(stars.size() * sizeof(SkyVertex)),
                 stars.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkyVertex),
                          reinterpret_cast<void*>(offsetof(SkyVertex, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(SkyVertex),
                          reinterpret_cast<void*>(offsetof(SkyVertex, color)));
    glBindVertexArray(0);
}

void Renderer::setViewport(int width, int height) {
    m_fbWidth = width > 0 ? width : 1;
    m_fbHeight = height > 0 ? height : 1;
    glViewport(0, 0, m_fbWidth, m_fbHeight);
}

void Renderer::setUIScale(float scale) {
    m_uiScale = std::clamp(scale, 1.0f, 12.0f);
}

void Renderer::beginFrame(const glm::vec3& clearColor) {
    m_stats.resetFrame();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawSky(const Camera& camera, float timeOfDay,
                       const glm::vec3& /*skyColor*/, const glm::vec3& /*fogColor*/, float sunlight) {
    const float aspect = static_cast<float>(m_fbWidth) / static_cast<float>(m_fbHeight);
    const glm::mat4 viewNoTrans = glm::mat4(glm::mat3(camera.viewMatrix()));
    const glm::mat4 vp = camera.projectionMatrix(aspect) * viewNoTrans;

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_skyShader.use();
    m_skyShader.setMat4("uVP", vp);

    // 1. Draw Stars at night
    const float starVisibility = glm::clamp((1.0f - sunlight * 1.4f), 0.0f, 1.0f);
    if (starVisibility > 0.01f && m_starCount > 0) {
        const float starAngle = timeOfDay * glm::two_pi<float>();
        const glm::mat4 starRot = glm::rotate(glm::mat4(1.0f), starAngle * 0.5f, glm::vec3(0.0f, 1.0f, 0.2f));
        m_skyShader.setMat4("uVP", vp * starRot);

        glPointSize(2.5f);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending for crisp twinkling stars
        glBindVertexArray(m_starVao);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_starCount));
        glBindVertexArray(0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        m_skyShader.setMat4("uVP", vp);
    }

    // 2. Build and draw celestial billboards (Sun, Moon, Sunset/Sunrise Horizon Glow)
    std::vector<SkyVertex> celestialVertices;
    celestialVertices.reserve(36);

    const float angle = timeOfDay * glm::two_pi<float>();
    // Sun arcs East (X > 0) to West (X < 0) with a slight celestial tilt
    const glm::vec3 sunDir = glm::normalize(glm::vec3(std::cos(angle), std::sin(angle), 0.22f));
    const glm::vec3 moonDir = -sunDir;
    const float dist = 90.0f;

    auto addCelestialQuad = [&](const glm::vec3& dir, float size, const glm::vec4& col) {
        const glm::vec3 center = dir * dist;
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        if (std::abs(glm::dot(dir, up)) > 0.95f) up = glm::vec3(0.0f, 0.0f, 1.0f);
        const glm::vec3 right = glm::normalize(glm::cross(dir, up)) * (size * 0.5f);
        const glm::vec3 top = glm::normalize(glm::cross(right, dir)) * (size * 0.5f);

        const glm::vec3 p0 = center - right - top;
        const glm::vec3 p1 = center + right - top;
        const glm::vec3 p2 = center + right + top;
        const glm::vec3 p3 = center - right + top;

        celestialVertices.push_back({p0, col});
        celestialVertices.push_back({p1, col});
        celestialVertices.push_back({p2, col});
        celestialVertices.push_back({p0, col});
        celestialVertices.push_back({p2, col});
        celestialVertices.push_back({p3, col});
    };

    // Sun (golden core + radiant corona)
    if (sunDir.y > -0.30f) {
        const float sunAlpha = glm::clamp((sunDir.y + 0.30f) / 0.30f, 0.0f, 1.0f);
        addCelestialQuad(sunDir, 22.0f, glm::vec4(1.0f, 0.78f, 0.20f, 0.40f * sunAlpha));
        addCelestialQuad(sunDir, 13.0f, glm::vec4(1.0f, 0.98f, 0.55f, 1.0f * sunAlpha));
    }

    // Moon (silver/white core + soft lunar glow)
    if (moonDir.y > -0.30f) {
        const float moonAlpha = glm::clamp((moonDir.y + 0.30f) / 0.30f, 0.0f, 1.0f);
        addCelestialQuad(moonDir, 18.0f, glm::vec4(0.55f, 0.75f, 1.0f, 0.30f * moonAlpha));
        addCelestialQuad(moonDir, 11.0f, glm::vec4(0.92f, 0.95f, 1.0f, 0.95f * moonAlpha));
    }

    // Horizon Sunset / Sunrise Glow
    const float sunElev = std::sin(angle);
    if (std::abs(sunElev) < 0.28f) {
        const float glowFactor = 1.0f - (std::abs(sunElev) / 0.28f);
        const glm::vec3 horizDir = glm::normalize(glm::vec3(sunDir.x, 0.05f, sunDir.z));
        addCelestialQuad(horizDir, 55.0f, glm::vec4(0.95f, 0.42f, 0.18f, 0.45f * glowFactor));
    }

    if (!celestialVertices.empty()) {
        glBindVertexArray(m_skyVao);
        glBindBuffer(GL_ARRAY_BUFFER, m_skyVbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(celestialVertices.size() * sizeof(SkyVertex)),
                     celestialVertices.data(), GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(celestialVertices.size()));
        glBindVertexArray(0);

        ++m_stats.drawCalls;
        m_stats.triangles += static_cast<uint32_t>(celestialVertices.size() / 3);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}

void Renderer::drawWorld(const World& world, const Camera& camera,
                         const glm::vec3& fogColor, float fogStart, float fogEnd, float sunlight) {
    const float aspect = static_cast<float>(m_fbWidth) / static_cast<float>(m_fbHeight);
    const glm::mat4 viewProjection = camera.projectionMatrix(aspect) * camera.viewMatrix();

    Frustum frustum;
    frustum.update(viewProjection);

    m_chunkShader.use();
    m_chunkShader.setMat4("uVP", viewProjection);
    m_chunkShader.setVec3("uCamPos", camera.position());
    m_chunkShader.setVec3("uFogColor", fogColor);
    m_chunkShader.setFloat("uFogStart", fogStart);
    m_chunkShader.setFloat("uFogEnd", fogEnd);
    m_chunkShader.setFloat("uSunlight", sunlight);
    m_chunkShader.setInt("uAtlas", 0);

    m_atlas.bind(0);

    // Frustum-cull once per chunk and share the result between both passes.
    // Testing the same AABB twice would both waste work and double-count the
    // "culled" figure the F3 overlay reports.
    const std::vector<std::unique_ptr<Chunk>>& chunks = world.chunks();
    std::vector<uint8_t> visible(chunks.size(), 0);
    for (size_t i = 0; i < chunks.size(); ++i) {
        const Chunk& chunk = *chunks[i];
        const glm::vec3 minP(chunk.originX(), 0.0f, chunk.originZ());
        const glm::vec3 maxP(chunk.originX() + Chunk::W, static_cast<float>(Chunk::H), chunk.originZ() + Chunk::D);
        if (frustum.isBoxVisible(minP, maxP)) {
            visible[i] = 1;
            ++m_stats.chunksVisible;
        } else {
            ++m_stats.chunksCulled;
        }
    }

    // Pass 1: Opaque geometry (depth test, backface culling)
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    for (size_t i = 0; i < chunks.size(); ++i) {
        const Chunk& chunk = *chunks[i];
        if (!visible[i] || chunk.mesh.empty()) continue;

        chunk.mesh.draw();
        ++m_stats.drawCalls;
        m_stats.triangles += chunk.mesh.triangleCount();
        ++m_stats.chunksDrawn;
    }

    // Pass 2: Transparent/Cutout geometry (two-sided leaves, water, torches)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    for (size_t i = 0; i < chunks.size(); ++i) {
        const Chunk& chunk = *chunks[i];
        if (!visible[i] || chunk.transparentMesh.empty()) continue;

        chunk.transparentMesh.draw();
        ++m_stats.drawCalls;
        m_stats.triangles += chunk.transparentMesh.triangleCount();
    }
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);

    // Particles are one additive batch, six vertices (two triangles) each.
    const size_t particles = m_particles.particleCount();
    m_particles.render(camera, aspect);
    if (particles > 0) {
        ++m_stats.drawCalls;
        m_stats.triangles += static_cast<uint32_t>(particles * 2);
    }
}

void Renderer::drawEntities(const EntityManager& entityManager, const World& world, const Camera& camera,
                            const glm::vec3& fogColor, float fogStart, float fogEnd, float sunlight) {
    if (entityManager.mobs().empty()) return;

    static std::vector<Vertex> entityVertices;
    entityVertices.clear();
    entityManager.buildMesh(entityVertices, world);

    if (entityVertices.empty()) return;

    const float aspect = static_cast<float>(m_fbWidth) / static_cast<float>(m_fbHeight);
    const glm::mat4 viewProjection = camera.projectionMatrix(aspect) * camera.viewMatrix();

    m_chunkShader.use();
    m_chunkShader.setMat4("uVP", viewProjection);
    m_chunkShader.setVec3("uCamPos", camera.position());
    m_chunkShader.setVec3("uFogColor", fogColor);
    m_chunkShader.setFloat("uFogStart", fogStart);
    m_chunkShader.setFloat("uFogEnd", fogEnd);
    m_chunkShader.setFloat("uSunlight", sunlight);
    m_chunkShader.setInt("uAtlas", 0);

    m_atlas.bind(0);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glBindVertexArray(m_entityVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_entityVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(entityVertices.size() * sizeof(Vertex)),
                 entityVertices.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(entityVertices.size()));
    glBindVertexArray(0);

    ++m_stats.drawCalls;
    m_stats.triangles += static_cast<uint32_t>(entityVertices.size() / 3);
}

void Renderer::drawPlayer(const Player& player, const World& world, const Camera& camera,
                          const glm::vec3& fogColor, float fogStart, float fogEnd, float sunlight) {
    static std::vector<Vertex> playerVertices;
    playerVertices.clear();
    player.appendGeometry(playerVertices, world);

    if (playerVertices.empty()) return;

    const float aspect = static_cast<float>(m_fbWidth) / static_cast<float>(m_fbHeight);
    const glm::mat4 viewProjection = camera.projectionMatrix(aspect) * camera.viewMatrix();

    m_chunkShader.use();
    m_chunkShader.setMat4("uVP", viewProjection);
    m_chunkShader.setVec3("uCamPos", camera.position());
    m_chunkShader.setVec3("uFogColor", fogColor);
    m_chunkShader.setFloat("uFogStart", fogStart);
    m_chunkShader.setFloat("uFogEnd", fogEnd);
    m_chunkShader.setFloat("uSunlight", sunlight);
    m_chunkShader.setInt("uAtlas", 0);

    m_atlas.bind(0);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glBindVertexArray(m_entityVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_entityVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(playerVertices.size() * sizeof(Vertex)),
                 playerVertices.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(playerVertices.size()));
    glBindVertexArray(0);

    ++m_stats.drawCalls;
    m_stats.triangles += static_cast<uint32_t>(playerVertices.size() / 3);
}

void Renderer::drawFirstPersonArm(const Player& player, const World& world, const Camera& /*camera*/, float sunlight) {
    if (player.perspective() != Perspective::FirstPerson) return;

    static std::vector<Vertex> armVertices;
    armVertices.clear();
    player.appendFirstPersonArm(armVertices, world);

    if (armVertices.empty()) return;

    const float aspect = static_cast<float>(m_fbWidth) / static_cast<float>(m_fbHeight);
    // Fixed viewmodel projection: camera-space identity view
    const glm::mat4 proj = glm::perspective(glm::radians(70.0f), aspect, 0.02f, 10.0f);

    m_chunkShader.use();
    m_chunkShader.setMat4("uVP", proj);
    m_chunkShader.setVec3("uCamPos", glm::vec3(0.0f));
    m_chunkShader.setVec3("uFogColor", glm::vec3(0.0f));
    m_chunkShader.setFloat("uFogStart", 999.0f);
    m_chunkShader.setFloat("uFogEnd", 1000.0f);
    m_chunkShader.setFloat("uSunlight", sunlight);
    m_chunkShader.setInt("uAtlas", 0);

    m_atlas.bind(0);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClear(GL_DEPTH_BUFFER_BIT); // Ensure viewmodel is never occluded by near terrain

    glBindVertexArray(m_entityVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_entityVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(armVertices.size() * sizeof(Vertex)),
                 armVertices.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(armVertices.size()));
    glBindVertexArray(0);

    ++m_stats.drawCalls;
    m_stats.triangles += static_cast<uint32_t>(armVertices.size() / 3);
}

void Renderer::drawSelection(const Camera& camera, const glm::ivec3& block, BlockId blockId) {
    const float aspect = static_cast<float>(m_fbWidth) / static_cast<float>(m_fbHeight);
    const glm::mat4 viewProjection = camera.projectionMatrix(aspect) * camera.viewMatrix();

    const BlockBounds bounds = blockBounds(blockId);
    const glm::vec3 size = bounds.maxOffset - bounds.minOffset;

    // Slight expansion (0.002f) matching Minecraft's bounding box expansion:
    // sits 0.001f in front of the front face so lines pass depth test without z-fighting,
    // while occluded and back edges fail the depth test against solid blocks.
    const float eps = 0.002f;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(block) + bounds.minOffset - glm::vec3(eps * 0.5f));
    model = glm::scale(model, size + glm::vec3(eps));

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_lineShader.use();
    m_lineShader.setMat4("uVP", viewProjection);
    m_lineShader.setMat4("uModel", model);
    m_lineShader.setVec3("uColor", glm::vec3(0.0f));
    glBindVertexArray(m_lineVao);
    glDrawArrays(GL_LINES, 0, 24);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    ++m_stats.drawCalls;
}

void Renderer::drawBlockBreak(const Camera& camera, const glm::ivec3& block, BlockId blockId, int stage) {
    if (stage < 0 || stage > 9) return;

    const float aspect = static_cast<float>(m_fbWidth) / static_cast<float>(m_fbHeight);
    const glm::mat4 viewProjection = camera.projectionMatrix(aspect) * camera.viewMatrix();

    const BlockBounds bounds = blockBounds(blockId);
    const glm::vec3 bMin = glm::vec3(block) + bounds.minOffset;
    const glm::vec3 bMax = glm::vec3(block) + bounds.maxOffset;

    const float eps = 0.003f;
    const float x0 = bMin.x - eps, x1 = bMax.x + eps;
    const float y0 = bMin.y - eps, y1 = bMax.y + eps;
    const float z0 = bMin.z - eps, z1 = bMax.z + eps;

    const TextureTile tile = static_cast<TextureTile>(static_cast<int>(TextureTile::Destroy0) + stage);
    const int tileIdx = static_cast<int>(tile);
    const int tx = tileIdx % config::ATLAS_TILES;
    const int ty = tileIdx / config::ATLAS_TILES;
    const float u0 = static_cast<float>(tx) / static_cast<float>(config::ATLAS_TILES);
    const float v0 = static_cast<float>(ty) / static_cast<float>(config::ATLAS_TILES);
    const float u1 = u0 + 1.0f / static_cast<float>(config::ATLAS_TILES);
    const float v1 = v0 + 1.0f / static_cast<float>(config::ATLAS_TILES);

    const glm::vec2 tileMin(u0, v0);
    const glm::vec2 tileSize(u1 - u0, v1 - v0);

    Vertex v[36];
    int idx = 0;

    auto addQuad = [&](const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
                       const glm::vec3& n) {
        v[idx++] = Vertex{p0, n, {u0, v0}, tileMin, tileSize, 1.0f, 1.0f};
        v[idx++] = Vertex{p1, n, {u1, v0}, tileMin, tileSize, 1.0f, 1.0f};
        v[idx++] = Vertex{p2, n, {u1, v1}, tileMin, tileSize, 1.0f, 1.0f};
        v[idx++] = Vertex{p0, n, {u0, v0}, tileMin, tileSize, 1.0f, 1.0f};
        v[idx++] = Vertex{p2, n, {u1, v1}, tileMin, tileSize, 1.0f, 1.0f};
        v[idx++] = Vertex{p3, n, {u0, v1}, tileMin, tileSize, 1.0f, 1.0f};
    };

    // +Y (Top)
    addQuad({x0, y1, z1}, {x1, y1, z1}, {x1, y1, z0}, {x0, y1, z0}, {0.0f, 1.0f, 0.0f});
    // -Y (Bottom)
    addQuad({x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1}, {0.0f, -1.0f, 0.0f});
    // +Z (South)
    addQuad({x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}, {0.0f, 0.0f, 1.0f});
    // -Z (North)
    addQuad({x1, y0, z0}, {x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0}, {0.0f, 0.0f, -1.0f});
    // +X (East)
    addQuad({x1, y0, z1}, {x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1}, {1.0f, 0.0f, 0.0f});
    // -X (West)
    addQuad({x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0}, {-1.0f, 0.0f, 0.0f});

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_chunkShader.use();
    m_chunkShader.setMat4("uVP", viewProjection);
    m_chunkShader.setVec3("uCamPos", camera.position());
    m_chunkShader.setVec3("uFogColor", glm::vec3(0.0f));
    m_chunkShader.setFloat("uFogStart", 999.0f);
    m_chunkShader.setFloat("uFogEnd", 1000.0f);
    m_chunkShader.setFloat("uSunlight", 1.0f);
    m_chunkShader.setInt("uAtlas", 0);
    m_atlas.bind(0);

    glBindVertexArray(m_entityVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_entityVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    ++m_stats.drawCalls;
    m_stats.triangles += 12;
}

void Renderer::beginUI() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::endUI() {
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void Renderer::uploadUI(const UIVertex* vertices, int count) {
    m_uiShader.use();
    m_uiShader.setVec2("uScreen", glm::vec2(static_cast<float>(m_fbWidth),
                                            static_cast<float>(m_fbHeight)));
    glBindVertexArray(m_uiVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_uiVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(sizeof(UIVertex) * static_cast<size_t>(count)),
                 vertices, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, count);
    glBindVertexArray(0);

    // Single funnel for every UI quad (rects, gradients, triangles), so the
    // overlay's draw-call figure covers the whole 2D layer.
    ++m_stats.drawCalls;
    ++m_stats.uiDrawCalls;
    m_stats.triangles += static_cast<uint32_t>(count / 3);
}

void Renderer::drawRect(float x, float y, float w, float h, const glm::vec4& color) {
    const glm::vec2 bl(x, y);
    const glm::vec2 br(x + w, y);
    const glm::vec2 tr(x + w, y + h);
    const glm::vec2 tl(x, y + h);
    const UIVertex vertices[6] = {
        {bl, color}, {br, color}, {tr, color},
        {bl, color}, {tr, color}, {tl, color},
    };
    uploadUI(vertices, 6);
}

void Renderer::drawGradientRect(float x, float y, float w, float h,
                                const glm::vec4& bottom, const glm::vec4& top) {
    const glm::vec2 bl(x, y);
    const glm::vec2 br(x + w, y);
    const glm::vec2 tr(x + w, y + h);
    const glm::vec2 tl(x, y + h);
    const UIVertex vertices[6] = {
        {bl, bottom}, {br, bottom}, {tr, top},
        {bl, bottom}, {tr, top}, {tl, top},
    };
    uploadUI(vertices, 6);
}

void Renderer::drawTriangle(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c,
                            const glm::vec4& color) {
    const UIVertex vertices[3] = {
        {a, color}, {b, color}, {c, color},
    };
    uploadUI(vertices, 3);
}

float Renderer::textWidth(const std::string& text, float scale) const {
    return m_font.textWidth(text, scale);
}

void Renderer::drawText(float x, float y, const std::string& text, float scale,
                        const glm::vec4& color, float rotationRadians) {
    if (text.empty()) return;

    m_textShader.use();
    m_textShader.setVec2("uScreen", glm::vec2(static_cast<float>(m_fbWidth),
                                              static_cast<float>(m_fbHeight)));
    m_textShader.setVec4("uColor", color);
    m_textShader.setInt("uFont", 0);
    m_font.bind(0);

    const float cellW = Font::CellWidth * scale;
    const float glyphW = Font::GlyphWidth * scale;
    const float glyphH = Font::GlyphHeight * scale;

    // Rotation pivot: centre of the whole string (y is the text's top edge).
    const glm::vec2 pivot(x + static_cast<float>(text.size()) * cellW * 0.5f,
                          y - glyphH * 0.5f);
    const float cs = std::cos(rotationRadians);
    const float sn = std::sin(rotationRadians);
    const auto transform = [&](glm::vec2 p) {
        if (rotationRadians == 0.0f) return p;
        const glm::vec2 d = p - pivot;
        return pivot + glm::vec2(d.x * cs - d.y * sn, d.x * sn + d.y * cs);
    };

    std::vector<TextVertex> vertices;
    vertices.reserve(text.size() * 6);

    for (size_t i = 0; i < text.size(); ++i) {
        const Font::Glyph* glyph = m_font.glyph(text[i]);
        if (!glyph || text[i] == ' ') continue;

        const float gx = x + static_cast<float>(i) * cellW;
        const glm::vec2 tl = transform({gx, y});
        const glm::vec2 tr = transform({gx + glyphW, y});
        const glm::vec2 br = transform({gx + glyphW, y - glyphH});
        const glm::vec2 bl = transform({gx, y - glyphH});

        vertices.push_back({tl, {glyph->u0, glyph->v0}});
        vertices.push_back({tr, {glyph->u1, glyph->v0}});
        vertices.push_back({br, {glyph->u1, glyph->v1}});
        vertices.push_back({tl, {glyph->u0, glyph->v0}});
        vertices.push_back({br, {glyph->u1, glyph->v1}});
        vertices.push_back({bl, {glyph->u0, glyph->v1}});
    }

    if (vertices.empty()) return;

    glBindVertexArray(m_textVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_textVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(TextVertex)),
                 vertices.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);

    ++m_stats.drawCalls;
    ++m_stats.uiDrawCalls;
    m_stats.triangles += static_cast<uint32_t>(vertices.size() / 3);
}

void Renderer::drawTexturedRect(float x, float y, float w, float h, TextureTile tile,
                                const glm::vec4& tint) {
    const int tileIdx = static_cast<int>(tile);
    const int tileCol = tileIdx % config::ATLAS_TILES;
    const int tileRow = tileIdx / config::ATLAS_TILES;
    const float tileUV = 1.0f / static_cast<float>(config::ATLAS_TILES);
    const float u0 = static_cast<float>(tileCol) * tileUV;
    const float u1 = u0 + tileUV;
    const float v0 = static_cast<float>(tileRow) * tileUV;
    const float v1 = v0 + tileUV;

    const glm::vec2 bl(x, y);
    const glm::vec2 br(x + w, y);
    const glm::vec2 tr(x + w, y + h);
    const glm::vec2 tl(x, y + h);

    const SpriteVertex vertices[6] = {
        {bl, {u0, v0}},
        {br, {u1, v0}},
        {tr, {u1, v1}},
        {bl, {u0, v0}},
        {tr, {u1, v1}},
        {tl, {u0, v1}},
    };

    m_spriteShader.use();
    m_spriteShader.setVec2("uScreen", glm::vec2(static_cast<float>(m_fbWidth),
                                                static_cast<float>(m_fbHeight)));
    m_spriteShader.setVec4("uTint", tint);
    m_spriteShader.setInt("uTexture", 0);
    m_atlas.bind(0);

    glBindVertexArray(m_spriteVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_spriteVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    ++m_stats.drawCalls;
    ++m_stats.uiDrawCalls;
    m_stats.triangles += 2;
}

void Renderer::drawUnderwaterOverlay(float time) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_underwaterShader.use();
    m_underwaterShader.setFloat("uTime", time);
    m_underwaterShader.setFloat("uAspect", static_cast<float>(m_fbWidth) / static_cast<float>(m_fbHeight));

    glBindVertexArray(m_underwaterVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void Renderer::drawBlockIcon(float x, float y, float w, float h, BlockId id) {
    if (isAir(id)) return;
    const BlockDef& def = blockDef(id);
    drawTexturedRect(x, y, w, h, def.side);
}

void Renderer::drawDurabilityBar(float x, float y, float w, float h, int durability, int maxDurability) {
    if (maxDurability <= 0 || durability <= 0) return;
    const float pct = std::clamp(static_cast<float>(durability) / static_cast<float>(maxDurability), 0.0f, 1.0f);
    drawRect(x, y, w, h, glm::vec4(0.04f, 0.04f, 0.05f, 1.0f));
    glm::vec4 barCol = (pct > 0.5f) ? glm::vec4(0.2f, 0.85f, 0.25f, 1.0f) :
                       ((pct > 0.2f) ? glm::vec4(0.9f, 0.8f, 0.15f, 1.0f) : glm::vec4(0.95f, 0.2f, 0.2f, 1.0f));
    drawRect(x, y, w * pct, h, barCol);
}

void Renderer::drawHud(int selectedSlot, const ItemSlot* hotbar, int slotCount) {
    const float s = m_uiScale;

    beginUI();

    // Crosshair (compact, high-contrast across all GUI scales).
    const float cx = static_cast<float>(m_fbWidth) * 0.5f;
    const float cy = static_cast<float>(m_fbHeight) * 0.5f;
    const float arm = std::max(3.0f, 2.5f * std::max(1.0f, s * 0.5f));
    const float thick = std::max(1.0f, s >= 3.0f ? 2.0f : 1.0f);
    const float outline = 1.0f;

    const glm::vec4 darkOutline(0.02f, 0.02f, 0.02f, 0.75f);
    const glm::vec4 innerColor(0.96f, 0.96f, 0.96f, 0.95f);

    // Dark outline wrapper
    drawRect(cx - arm - outline, cy - (thick * 0.5f) - outline, (arm * 2.0f) + (outline * 2.0f), thick + (outline * 2.0f), darkOutline);
    drawRect(cx - (thick * 0.5f) - outline, cy - arm - outline, thick + (outline * 2.0f), (arm * 2.0f) + (outline * 2.0f), darkOutline);

    // Inner bright core
    drawRect(cx - arm, cy - thick * 0.5f, arm * 2.0f, thick, innerColor);
    drawRect(cx - thick * 0.5f, cy - arm, thick, arm * 2.0f, innerColor);

    // Hotbar.
    const float slot = 16.0f * s;
    const float gap = 1.5f * s;
    const float margin = 6.0f * s;
    const float border = 1.0f * s;
    const float totalWidth = slotCount * slot + (slotCount - 1) * gap;
    const float startX = (static_cast<float>(m_fbWidth) - totalWidth) * 0.5f;

    for (int i = 0; i < slotCount; ++i) {
        const float x = startX + i * (slot + gap);
        if (i == selectedSlot) {
            drawRect(x - gap, margin - gap, slot + 2.0f * gap, slot + 2.0f * gap, glm::vec4(1.0f));
        }
        drawRect(x, margin, slot, slot, glm::vec4(0.10f, 0.10f, 0.10f, 1.0f));

        if (!hotbar[i].empty()) {
            drawBlockIcon(x + border, margin + border, slot - 2.0f * border, slot - 2.0f * border, hotbar[i].id);

            // Stack count
            if (hotbar[i].count > 1) {
                const std::string cntStr = std::to_string(hotbar[i].count);
                const float cntScale = std::max(1.0f, s * 0.60f);
                const float cw = textWidth(cntStr, cntScale);
                drawText(x + slot - cw - 1.0f * s + 1.0f, margin + textHeight(cntScale) + 1.0f * s - 1.0f,
                         cntStr, cntScale, glm::vec4(0.0f, 0.0f, 0.0f, 0.85f));
                drawText(x + slot - cw - 1.0f * s, margin + textHeight(cntScale) + 1.0f * s,
                         cntStr, cntScale, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            }

            // Durability bar for tools
            if (isTool(hotbar[i].id) && hotbar[i].durability > 0) {
                const float barH = std::max(1.5f, 1.5f * s);
                drawDurabilityBar(x + border, margin + border, slot - 2.0f * border, barH,
                                  hotbar[i].durability, maxToolDurability(hotbar[i].id));
            }
        }
    }

    endUI();
}

void Renderer::drawDebugOverlay(const std::vector<std::string>& lines) {
    if (lines.empty()) return;

    // The overlay must not count itself, otherwise the draw-call figure it
    // displays would include the ~lines.size()+1 quads it is made of. Snapshot
    // the counters and restore them afterwards.
    const int savedDrawCalls = m_stats.drawCalls;
    const int savedUiCalls = m_stats.uiDrawCalls;
    const uint32_t savedTriangles = m_stats.triangles;

    // Deliberately uses its own scale rather than m_uiScale: the debug readout
    // has to stay legible even at GUI Scale 1.
    const float scale = std::clamp(m_uiScale * 0.75f, 1.0f, 2.0f);
    const float lineHeight = textHeight(scale) + 2.0f;
    const float pad = 4.0f;

    float widest = 0.0f;
    for (const std::string& line : lines) {
        widest = std::max(widest, textWidth(line, scale));
    }

    const float panelW = widest + pad * 2.0f;
    const float panelH = lineHeight * static_cast<float>(lines.size()) + pad * 2.0f;
    const float x = 4.0f;
    const float y = static_cast<float>(m_fbHeight) - panelH - 4.0f;

    beginUI();
    drawRect(x, y, panelW, panelH, glm::vec4(0.0f, 0.0f, 0.0f, 0.62f));

    float textY = y + panelH - pad - textHeight(scale);
    for (const std::string& line : lines) {
        // Dark backing for legibility against bright terrain.
        drawText(x + pad + 1.0f, textY - 1.0f, line, scale, glm::vec4(0.0f, 0.0f, 0.0f, 0.75f));
        drawText(x + pad, textY, line, scale, glm::vec4(1.0f, 1.0f, 1.0f, 0.95f));
        textY -= lineHeight;
    }
    endUI();

    m_stats.drawCalls = savedDrawCalls;
    m_stats.uiDrawCalls = savedUiCalls;
    m_stats.triangles = savedTriangles;
}

void Renderer::drawInventory(int selectedHotbarSlot, const ItemSlot* hotbar, int hotbarCount,
                             const ItemSlot* inventory, int invCount,
                             const ItemSlot* craftGrid, const ItemSlot& craftResult,
                             const ItemSlot& heldItem, const glm::vec2& mousePos) {
    const float s = m_uiScale;
    const float slot = 18.0f * s;
    const float gap = 2.5f * s;
    const float border = 1.5f * s;

    const int cols = 8;
    const int mainRows = 3;

    const float gridW = cols * slot + (cols - 1) * gap;
    const float mainH = mainRows * slot + (mainRows - 1) * gap;
    const float hotbarH = slot;

    const float pad = 12.0f * s;
    const float headerH = 20.0f * s;
    const float craftSectionH = 2.0f * slot + gap + 16.0f * s;
    const float labelH = 12.0f * s;
    const float sectionGap = 8.0f * s;
    const float footerH = 14.0f * s;

    const float containerW = gridW + 2.0f * pad;
    const float containerH = pad + headerH + craftSectionH + sectionGap + mainH + sectionGap + labelH + hotbarH + footerH + pad;

    const float cx = static_cast<float>(m_fbWidth) * 0.5f;
    const float cy = static_cast<float>(m_fbHeight) * 0.5f;

    const float left = cx - containerW * 0.5f;
    const float bottom = cy - containerH * 0.5f;

    beginUI();

    // 1. Fullscreen dark backdrop dimming
    drawRect(0.0f, 0.0f, static_cast<float>(m_fbWidth), static_cast<float>(m_fbHeight),
             glm::vec4(0.0f, 0.0f, 0.0f, 0.60f));

    // 2. Main Console Modal Container (3D Beveled Slate Frame)
    drawRect(left - 2.0f * s, bottom - 2.0f * s, containerW + 4.0f * s, containerH + 4.0f * s,
             glm::vec4(0.08f, 0.08f, 0.10f, 1.0f));
    drawRect(left - 1.0f * s, bottom, containerW + 2.0f * s, containerH + 1.0f * s,
             glm::vec4(0.38f, 0.38f, 0.44f, 1.0f));
    drawRect(left, bottom - 1.0f * s, containerW + 1.0f * s, containerH,
             glm::vec4(0.12f, 0.12f, 0.15f, 1.0f));
    drawGradientRect(left, bottom, containerW, containerH,
                     glm::vec4(0.17f, 0.17f, 0.21f, 0.98f),
                     glm::vec4(0.22f, 0.22f, 0.27f, 0.98f));

    // 3. Header: Survival Inventory & Crafting
    const float headerTop = bottom + containerH - pad;
    const float titleScale = std::max(1.0f, s * 0.85f);
    drawText(left + pad, headerTop - 1.0f * s, "SURVIVAL INVENTORY & CRAFTING", titleScale, glm::vec4(1.0f, 0.86f, 0.32f, 1.0f));
    drawRect(left + pad, headerTop - 14.0f * s, gridW, 1.5f * s, glm::vec4(0.98f, 0.78f, 0.22f, 0.85f));

    std::string hoveredName;

    // 4. Crafting Section (2x2 Grid + Arrow + Result Slot)
    const float craftSectionTop = headerTop - headerH;
    const float craftSectionY = craftSectionTop - craftSectionH;
    const float craftLabelScale = std::max(1.0f, s * 0.72f);
    drawText(left + pad, craftSectionTop - 2.0f * s, "Crafting (2x2 Grid)", craftLabelScale, glm::vec4(0.85f, 0.85f, 0.88f, 1.0f));

    const float craftGridLeft = left + pad + 24.0f * s;
    const float craftGridTop = craftSectionTop - 14.0f * s;

    // Helper lambda to draw any interactive item slot
    auto drawSlot = [&](float x, float y, const ItemSlot& item, bool isHotbar, int hotbarIdx, bool isCraftResult) {
        const bool hovered = (mousePos.x >= x && mousePos.x <= x + slot &&
                              mousePos.y >= y && mousePos.y <= y + slot);
        const bool active = (isHotbar && hotbarIdx == selectedHotbarSlot);

        // Sunken bevel
        drawRect(x, y, slot, slot, glm::vec4(0.08f, 0.08f, 0.10f, 1.0f));
        drawRect(x + border, y, slot - border, slot - border, glm::vec4(0.32f, 0.32f, 0.38f, 1.0f));
        drawRect(x + border, y + border, slot - 2.0f * border, slot - 2.0f * border,
                 isCraftResult ? glm::vec4(0.18f, 0.16f, 0.12f, 1.0f) : glm::vec4(0.13f, 0.13f, 0.16f, 1.0f));

        if (isCraftResult) {
            // Gold border highlight for craft result
            drawRect(x - 1.0f * s, y - 1.0f * s, slot + 2.0f * s, slot + 2.0f * s, glm::vec4(0.85f, 0.70f, 0.20f, 0.6f));
        }

        if (hovered) {
            drawRect(x - 1.5f * s, y - 1.5f * s, slot + 3.0f * s, slot + 3.0f * s, glm::vec4(1.0f, 0.84f, 0.22f, 1.0f));
            drawRect(x, y, slot, slot, glm::vec4(1.0f, 1.0f, 1.0f, 0.25f));
            if (!item.empty()) {
                hoveredName = blockDef(item.id).name;
                if (isHotbar) hoveredName += " (Slot " + std::to_string(hotbarIdx + 1) + ")";
                if (isTool(item.id)) {
                    hoveredName += " [" + std::to_string(item.durability) + "/" +
                                   std::to_string(maxToolDurability(item.id)) + "]";
                }
            }
        } else if (active) {
            drawRect(x - 1.5f * s, y - 1.5f * s, slot + 3.0f * s, slot + 3.0f * s, glm::vec4(0.95f, 0.95f, 0.95f, 1.0f));
        }

        if (!item.empty()) {
            const float iconPad = 2.0f * s;
            drawBlockIcon(x + iconPad, y + iconPad, slot - 2.0f * iconPad, slot - 2.0f * iconPad, item.id);

            // Stack count
            if (item.count > 1) {
                const std::string cntStr = std::to_string(item.count);
                const float cntScale = std::max(1.0f, s * 0.60f);
                const float cw = textWidth(cntStr, cntScale);
                drawText(x + slot - cw - 1.0f * s + 1.0f, y + textHeight(cntScale) + 1.0f * s - 1.0f,
                         cntStr, cntScale, glm::vec4(0.0f, 0.0f, 0.0f, 0.85f));
                drawText(x + slot - cw - 1.0f * s, y + textHeight(cntScale) + 1.0f * s,
                         cntStr, cntScale, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            }

            // Durability bar
            if (isTool(item.id) && item.durability > 0) {
                const float barH = std::max(1.5f, 1.5f * s);
                drawDurabilityBar(x + border, y + border, slot - 2.0f * border, barH,
                                  item.durability, maxToolDurability(item.id));
            }
        }
    };

    // Draw 2x2 Craft Grid
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            const int idx = r * 2 + c;
            const float sx = craftGridLeft + c * (slot + gap);
            const float sy = craftGridTop - (r + 1) * slot - r * gap;
            drawSlot(sx, sy, craftGrid[idx], false, -1, false);
        }
    }

    // Arrow pointing to Result
    const float arrowX = craftGridLeft + 2.0f * (slot + gap) + 12.0f * s;
    const float arrowY = craftGridTop - slot - gap * 0.5f - 6.0f * s;
    drawText(arrowX, arrowY + 10.0f * s, "=>", std::max(1.0f, s * 1.0f), glm::vec4(0.95f, 0.85f, 0.35f, 0.95f));

    // Result Slot
    const float resultX = arrowX + 26.0f * s;
    const float resultY = craftGridTop - slot - gap * 0.5f - slot * 0.5f;
    drawSlot(resultX, resultY, craftResult, false, -1, true);

    // 5. Main Inventory Grid (3x8)
    const float gridLeft = left + pad;
    const float mainGridTop = craftSectionY - sectionGap;

    for (int row = 0; row < mainRows; ++row) {
        for (int col = 0; col < cols; ++col) {
            const int idx = row * cols + col;
            if (idx >= invCount) break;
            const float x = gridLeft + col * (slot + gap);
            const float y = mainGridTop - (row + 1) * slot - row * gap;
            drawSlot(x, y, inventory[idx], false, -1, false);
        }
    }

    // 6. Section Divider & Hotbar Header
    const float dividerY = mainGridTop - mainH - sectionGap * 0.5f;
    drawRect(gridLeft, dividerY, gridW, 1.0f * s, glm::vec4(0.08f, 0.08f, 0.10f, 1.0f));
    drawRect(gridLeft, dividerY - 1.0f * s, gridW, 1.0f * s, glm::vec4(0.32f, 0.32f, 0.38f, 0.70f));

    const float subLabelTop = dividerY - 3.0f * s;
    drawText(gridLeft, subLabelTop, "Hotbar", craftLabelScale, glm::vec4(0.85f, 0.85f, 0.88f, 1.0f));

    // 7. Hotbar Slots Grid (1x8)
    const float hotbarY = subLabelTop - labelH - slot;
    for (int i = 0; i < hotbarCount && i < cols; ++i) {
        const float x = gridLeft + i * (slot + gap);
        drawSlot(x, hotbarY, hotbar[i], true, i, false);
    }

    // 8. Floating Hover Tooltip
    if (!hoveredName.empty()) {
        const float tipScale = std::max(1.0f, s * 0.75f);
        const float tipW = textWidth(hoveredName, tipScale);
        const float tipH = textHeight(tipScale);
        const float tipX = std::min(mousePos.x + 10.0f * s, static_cast<float>(m_fbWidth) - tipW - 10.0f * s);
        const float tipY = std::max(mousePos.y + tipH + 4.0f * s, tipH + 10.0f * s);

        drawRect(tipX - 4.0f * s, tipY - tipH - 4.0f * s, tipW + 8.0f * s, tipH + 8.0f * s, glm::vec4(0.08f, 0.04f, 0.12f, 0.96f));
        drawRect(tipX - 3.0f * s, tipY - tipH - 3.0f * s, tipW + 6.0f * s, tipH + 6.0f * s, glm::vec4(0.35f, 0.10f, 0.65f, 0.85f));
        drawRect(tipX - 2.0f * s, tipY - tipH - 2.0f * s, tipW + 4.0f * s, tipH + 4.0f * s, glm::vec4(0.08f, 0.04f, 0.12f, 0.96f));
        drawText(tipX, tipY, hoveredName, tipScale, glm::vec4(1.0f, 0.90f, 0.40f, 1.0f));
    }

    // 9. Held Item on Mouse Cursor
    if (!heldItem.empty()) {
        const float itemSize = 16.0f * s;
        drawRect(mousePos.x - itemSize * 0.5f + 1.5f * s, mousePos.y - itemSize * 0.5f - 1.5f * s,
                 itemSize, itemSize, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
        drawBlockIcon(mousePos.x - itemSize * 0.5f, mousePos.y - itemSize * 0.5f,
                      itemSize, itemSize, heldItem.id);
        if (heldItem.count > 1) {
            const std::string cntStr = std::to_string(heldItem.count);
            const float cntScale = std::max(1.0f, s * 0.60f);
            drawText(mousePos.x + itemSize * 0.3f, mousePos.y - itemSize * 0.3f,
                     cntStr, cntScale, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
    }

    // 10. Footer Prompt
    const std::string footerText = "Left Click: Swap/Craft     Right Click: Place 1     1-8: Quick Hotbar     E/Esc: Close";
    const float hintScale = std::max(1.0f, s * 0.70f);
    const float hintW = textWidth(footerText, hintScale);
    drawText((static_cast<float>(m_fbWidth) - hintW) * 0.5f, bottom + 6.0f * s,
             footerText, hintScale, glm::vec4(0.85f, 0.85f, 0.88f, 0.85f));

    endUI();
}

} // namespace vox

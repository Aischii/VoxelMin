#include "render/Renderer.hpp"
#include "core/Config.hpp"
#include "core/Log.hpp"
#include "entity/EntityManager.hpp"
#include "player/Player.hpp"
#include "render/Camera.hpp"
#include "render/Frustum.hpp"
#include "world/Block.hpp"
#include "world/Chunk.hpp"
#include "world/CraftingRecipes.hpp"
#include "world/World.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
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
    if (!m_skyDomeShader.loadFromFiles(resolveAsset("assets/shaders/skydome.vert"),
                                       resolveAsset("assets/shaders/skydome.frag"))) {
        return false;
    }

    m_atlas.createAtlas();
    const std::string fontPath = resolveAsset("assets/fonts/born2bsporty-fs.otf");
    if (!m_font.build(fontPath)) {
        if (!m_font.build()) return false;
    }
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
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, torchLight)));
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
    m_skyDomeShader.destroy();
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
    if (m_skyDomeVbo)    { glDeleteBuffers(1, &m_skyDomeVbo); m_skyDomeVbo = 0; }
    if (m_skyDomeEbo)    { glDeleteBuffers(1, &m_skyDomeEbo); m_skyDomeEbo = 0; }
    if (m_skyDomeVao)    { glDeleteVertexArrays(1, &m_skyDomeVao); m_skyDomeVao = 0; }
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

    // Sky dome: UV sphere centred on the camera. The fragment shader turns
    // the view direction into a zenith/horizon gradient, so the sky changes
    // with the direction the player looks instead of being a flat clear color.
    constexpr float kDomeRadius = 300.0f; // camera far plane is 400
    constexpr float kPi = 3.14159265358979323846f;
    constexpr int kSegX = 24;
    constexpr int kSegY = 12;

    std::vector<glm::vec3> domeVerts;
    domeVerts.reserve((kSegX + 1) * (kSegY + 1));
    for (int y = 0; y <= kSegY; ++y) {
        const float phi = (static_cast<float>(y) / kSegY) * kPi;
        for (int x = 0; x <= kSegX; ++x) {
            const float theta = (static_cast<float>(x) / kSegX) * 2.0f * kPi;
            domeVerts.push_back({
                kDomeRadius * std::sin(phi) * std::cos(theta),
                kDomeRadius * std::cos(phi),
                kDomeRadius * std::sin(phi) * std::sin(theta),
            });
        }
    }

    std::vector<uint32_t> domeIndices;
    domeIndices.reserve(static_cast<size_t>(kSegX) * kSegY * 6);
    for (int y = 0; y < kSegY; ++y) {
        for (int x = 0; x < kSegX; ++x) {
            const uint32_t a = static_cast<uint32_t>(y * (kSegX + 1) + x);
            const uint32_t b = a + static_cast<uint32_t>(kSegX + 1);
            domeIndices.push_back(a);
            domeIndices.push_back(b);
            domeIndices.push_back(a + 1);
            domeIndices.push_back(a + 1);
            domeIndices.push_back(b);
            domeIndices.push_back(b + 1);
        }
    }

    m_skyDomeIndexCount = static_cast<int>(domeIndices.size());
    glGenVertexArrays(1, &m_skyDomeVao);
    glGenBuffers(1, &m_skyDomeVbo);
    glGenBuffers(1, &m_skyDomeEbo);
    glBindVertexArray(m_skyDomeVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_skyDomeVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(domeVerts.size() * sizeof(glm::vec3)),
                 domeVerts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_skyDomeEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(domeIndices.size() * sizeof(uint32_t)),
                 domeIndices.data(), GL_STATIC_DRAW);
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
                       const glm::vec3& skyColor, const glm::vec3& /*fogColor*/, float sunlight) {
    const float aspect = static_cast<float>(m_fbWidth) / static_cast<float>(m_fbHeight);
    const glm::mat4 viewNoTrans = glm::mat4(glm::mat3(camera.viewMatrix()));
    const glm::mat4 vp = camera.projectionMatrix(aspect) * viewNoTrans;
    // The dome follows the camera position, so it needs the full view matrix.
    const glm::mat4 vpFull = camera.projectionMatrix(aspect) * camera.viewMatrix();

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 0. Sky dome gradient (zenith/horizon/ground) so the sky varies with the
    // direction the camera faces. Horizon matches the clear/fog colour so
    // distant terrain blends seamlessly into the sky.
    if (m_skyDomeIndexCount > 0) {
        m_skyDomeShader.use();
        m_skyDomeShader.setMat4("uVP", vpFull);
        m_skyDomeShader.setVec3("uCamPos", camera.position());
        m_skyDomeShader.setVec3("uZenith", skyColor * 0.70f);
        m_skyDomeShader.setVec3("uHorizon", skyColor);
        m_skyDomeShader.setVec3("uGround", skyColor * 0.30f);
        glBindVertexArray(m_skyDomeVao);
        glDrawElements(GL_TRIANGLES, m_skyDomeIndexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
        ++m_stats.drawCalls;
        m_stats.triangles += static_cast<uint32_t>(m_skyDomeIndexCount / 3);
    }

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

    auto addCelestialQuad = [&](const glm::vec3& dir, float size, const glm::vec4& col,
                                const glm::vec2& offset = glm::vec2(0.0f)) {
        const glm::vec3 center = dir * dist;
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        if (std::abs(glm::dot(dir, up)) > 0.95f) up = glm::vec3(0.0f, 0.0f, 1.0f);
        const glm::vec3 rUnit = glm::normalize(glm::cross(dir, up));
        const glm::vec3 tUnit = glm::normalize(glm::cross(rUnit, dir));

        const glm::vec3 offsetCenter = center + (rUnit * offset.x) + (tUnit * offset.y);
        const glm::vec3 right = rUnit * (size * 0.5f);
        const glm::vec3 top = tUnit * (size * 0.5f);

        const glm::vec3 p0 = offsetCenter - right - top;
        const glm::vec3 p1 = offsetCenter + right - top;
        const glm::vec3 p2 = offsetCenter + right + top;
        const glm::vec3 p3 = offsetCenter - right + top;

        celestialVertices.push_back({p0, col});
        celestialVertices.push_back({p1, col});
        celestialVertices.push_back({p2, col});
        celestialVertices.push_back({p0, col});
        celestialVertices.push_back({p2, col});
        celestialVertices.push_back({p3, col});
    };

    // Sun (Minecraft-style brilliant square body + radiant golden halos)
    if (sunDir.y > -0.35f) {
        const float sunAlpha = glm::clamp((sunDir.y + 0.35f) / 0.35f, 0.0f, 1.0f);
        // Outer soft celestial glow
        addCelestialQuad(sunDir, 72.0f, glm::vec4(1.0f, 0.60f, 0.12f, 0.20f * sunAlpha));
        // Golden corona halo
        addCelestialQuad(sunDir, 46.0f, glm::vec4(1.0f, 0.82f, 0.20f, 0.52f * sunAlpha));
        // Brilliant square Sun core
        addCelestialQuad(sunDir, 28.0f, glm::vec4(1.0f, 1.0f, 0.95f, 1.0f * sunAlpha));
    }

    // Moon (Minecraft-style square Moon + lunar crater detail + silver-blue aura)
    if (moonDir.y > -0.35f) {
        const float moonAlpha = glm::clamp((moonDir.y + 0.35f) / 0.35f, 0.0f, 1.0f);
        // Outer soft lunar glow
        addCelestialQuad(moonDir, 65.0f, glm::vec4(0.30f, 0.50f, 0.95f, 0.18f * moonAlpha));
        // Glowing lunar aura
        addCelestialQuad(moonDir, 42.0f, glm::vec4(0.48f, 0.68f, 1.0f, 0.32f * moonAlpha));
        // Silver-white square Moon body
        addCelestialQuad(moonDir, 24.0f, glm::vec4(0.92f, 0.95f, 1.0f, 0.98f * moonAlpha));
        // Lunar crater details
        addCelestialQuad(moonDir, 7.0f, glm::vec4(0.65f, 0.70f, 0.80f, 0.95f * moonAlpha), glm::vec2(-4.5f, 3.5f));
        addCelestialQuad(moonDir, 8.5f, glm::vec4(0.62f, 0.68f, 0.78f, 0.95f * moonAlpha), glm::vec2(3.5f, -4.0f));
        addCelestialQuad(moonDir, 5.5f, glm::vec4(0.68f, 0.74f, 0.84f, 0.95f * moonAlpha), glm::vec2(4.5f, 4.5f));
    }

    // Horizon Sunset / Sunrise Glow
    const float sunElev = std::sin(angle);
    if (std::abs(sunElev) < 0.30f) {
        const float glowFactor = 1.0f - (std::abs(sunElev) / 0.30f);
        const glm::vec3 horizDir = glm::normalize(glm::vec3(sunDir.x, 0.04f, sunDir.z));
        addCelestialQuad(horizDir, 65.0f, glm::vec4(0.95f, 0.44f, 0.18f, 0.45f * glowFactor));
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
                         const glm::vec3& fogColor, float fogStart, float fogEnd, float sunlight,
                         float heldLightIntensity, const glm::vec3& heldLightPos) {
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
    m_chunkShader.setFloat("uIsBackrooms", world.currentDimension() == DimensionId::Backrooms ? 1.0f : 0.0f);
    m_chunkShader.setFloat("uTime", static_cast<float>(glfwGetTime()));
    m_chunkShader.setVec3("uHeldLightPos", heldLightPos);
    m_chunkShader.setFloat("uHeldLightIntensity", heldLightIntensity);
    m_chunkShader.setInt("uAtlas", 0);

    m_atlas.bind(0);

    // Frustum-cull once per chunk and share the result between both passes.
    // Testing the same AABB twice would both waste work and double-count the
    // "culled" figure the F3 overlay reports.
    const std::vector<Chunk*>& chunks = world.loadedChunks();
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
                            const glm::vec3& fogColor, float fogStart, float fogEnd, float sunlight,
                            float heldLightIntensity, const glm::vec3& heldLightPos) {
    if (entityManager.mobs().empty() && entityManager.items().empty()) return;

    const float aspect = static_cast<float>(m_fbWidth) / static_cast<float>(m_fbHeight);
    const glm::mat4 viewProjection = camera.projectionMatrix(aspect) * camera.viewMatrix();

    Frustum frustum;
    frustum.update(viewProjection);

    static std::vector<Vertex> entityVertices;
    entityVertices.clear();
    entityManager.buildMesh(entityVertices, world, &frustum);

    if (entityVertices.empty()) return;

    m_chunkShader.use();
    m_chunkShader.setMat4("uVP", viewProjection);
    m_chunkShader.setVec3("uCamPos", camera.position());
    m_chunkShader.setVec3("uFogColor", fogColor);
    m_chunkShader.setFloat("uFogStart", fogStart);
    m_chunkShader.setFloat("uFogEnd", fogEnd);
    m_chunkShader.setFloat("uSunlight", sunlight);
    m_chunkShader.setFloat("uIsBackrooms", world.currentDimension() == DimensionId::Backrooms ? 1.0f : 0.0f);
    m_chunkShader.setFloat("uTime", static_cast<float>(glfwGetTime()));
    m_chunkShader.setVec3("uHeldLightPos", heldLightPos);
    m_chunkShader.setFloat("uHeldLightIntensity", heldLightIntensity);
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
                          const glm::vec3& fogColor, float fogStart, float fogEnd, float sunlight,
                          float heldLightIntensity, const glm::vec3& heldLightPos) {
    if (player.perspective() == Perspective::FirstPerson) return;

    const float aspect = static_cast<float>(m_fbWidth) / static_cast<float>(m_fbHeight);
    const glm::mat4 viewProjection = camera.projectionMatrix(aspect) * camera.viewMatrix();

    Frustum frustum;
    frustum.update(viewProjection);

    const glm::vec3 pos = player.position();
    const glm::vec3 minP(pos.x - 0.45f, pos.y, pos.z - 0.45f);
    const glm::vec3 maxP(pos.x + 0.45f, pos.y + 1.9f, pos.z + 0.45f);
    if (!frustum.isBoxVisible(minP, maxP)) return;

    static std::vector<Vertex> playerVertices;
    playerVertices.clear();
    player.appendGeometry(playerVertices, world);

    if (playerVertices.empty()) return;

    m_chunkShader.use();
    m_chunkShader.setMat4("uVP", viewProjection);
    m_chunkShader.setVec3("uCamPos", camera.position());
    m_chunkShader.setVec3("uFogColor", fogColor);
    m_chunkShader.setFloat("uFogStart", fogStart);
    m_chunkShader.setFloat("uFogEnd", fogEnd);
    m_chunkShader.setFloat("uSunlight", sunlight);
    m_chunkShader.setFloat("uIsBackrooms", world.currentDimension() == DimensionId::Backrooms ? 1.0f : 0.0f);
    m_chunkShader.setFloat("uTime", static_cast<float>(glfwGetTime()));
    m_chunkShader.setVec3("uHeldLightPos", heldLightPos);
    m_chunkShader.setFloat("uHeldLightIntensity", heldLightIntensity);
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

void Renderer::drawFirstPersonArm(const Player& player, const World& world, const Camera& /*camera*/, float sunlight,
                                 float heldLightIntensity, const glm::vec3& heldLightPos) {
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
    m_chunkShader.setFloat("uIsBackrooms", world.currentDimension() == DimensionId::Backrooms ? 1.0f : 0.0f);
    m_chunkShader.setFloat("uTime", static_cast<float>(glfwGetTime()));
    m_chunkShader.setVec3("uHeldLightPos", heldLightPos);
    m_chunkShader.setFloat("uHeldLightIntensity", heldLightIntensity);
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

void Renderer::drawBlockBreak(const Camera& camera, const glm::ivec3& block, BlockId blockId, int stage,
                              const World& world, float sunlight) {
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

    (void)world;
    (void)sunlight;

    Vertex v[36];
    int idx = 0;

    auto addQuad = [&](const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
                       const glm::vec3& n) {
        v[idx++] = Vertex{p0, n, {0.0f, 0.0f}, tileMin, tileSize, 1.0f, 1.0f, 0.0f};
        v[idx++] = Vertex{p1, n, {1.0f, 0.0f}, tileMin, tileSize, 1.0f, 1.0f, 0.0f};
        v[idx++] = Vertex{p2, n, {1.0f, 1.0f}, tileMin, tileSize, 1.0f, 1.0f, 0.0f};
        v[idx++] = Vertex{p0, n, {0.0f, 0.0f}, tileMin, tileSize, 1.0f, 1.0f, 0.0f};
        v[idx++] = Vertex{p2, n, {1.0f, 1.0f}, tileMin, tileSize, 1.0f, 1.0f, 0.0f};
        v[idx++] = Vertex{p3, n, {0.0f, 1.0f}, tileMin, tileSize, 1.0f, 1.0f, 0.0f};
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

    // Authentic multiplicative crack blend mode
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);

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

    const float totalW = m_font.textWidth(text, scale);
    const float totalH = m_font.textHeight(scale);

    // Rotation pivot: centre of the whole string
    const glm::vec2 pivot(x + totalW * 0.5f, y - totalH * 0.5f);
    const float cs = std::cos(rotationRadians);
    const float sn = std::sin(rotationRadians);
    const auto transform = [&](glm::vec2 p) {
        if (rotationRadians == 0.0f) return p;
        const glm::vec2 d = p - pivot;
        return pivot + glm::vec2(d.x * cs - d.y * sn, d.x * sn + d.y * cs);
    };

    std::vector<TextVertex> vertices;
    vertices.reserve(text.size() * 6);

    float curX = x;
    for (char c : text) {
        const Font::Glyph* glyph = m_font.glyph(c);
        if (!glyph) {
            curX += Font::CellWidth * scale;
            continue;
        }

        if (c != ' ') {
            const float gx0 = curX + glyph->x0 * scale;
            const float gx1 = curX + glyph->x1 * scale;
            const float gy0 = y + glyph->y0 * scale;
            const float gy1 = y + glyph->y1 * scale;

            const glm::vec2 tl = transform({gx0, gy0});
            const glm::vec2 tr = transform({gx1, gy0});
            const glm::vec2 br = transform({gx1, gy1});
            const glm::vec2 bl = transform({gx0, gy1});

            vertices.push_back({tl, {glyph->u0, glyph->v0}});
            vertices.push_back({tr, {glyph->u1, glyph->v0}});
            vertices.push_back({br, {glyph->u1, glyph->v1}});
            vertices.push_back({tl, {glyph->u0, glyph->v0}});
            vertices.push_back({br, {glyph->u1, glyph->v1}});
            vertices.push_back({bl, {glyph->u0, glyph->v1}});
        }

        curX += glyph->xadvance * scale;
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

void Renderer::drawBackroomsHorrorOverlay(float time) {
    beginUI();
    const float fbW = static_cast<float>(m_fbWidth);
    const float fbH = static_cast<float>(m_fbHeight);

    // 1. Dark liminal edge vignette
    const float thick = std::min(fbW, fbH) * 0.22f;
    const glm::vec4 vigSolid(0.04f, 0.035f, 0.015f, 0.45f);
    const glm::vec4 vigTrans(0.04f, 0.035f, 0.015f, 0.0f);

    drawGradientRect(0.0f, 0.0f, fbW, thick, vigSolid, vigTrans);
    drawGradientRect(0.0f, fbH - thick, fbW, thick, vigTrans, vigSolid);

    // 2. VHS tape tracking distortion glitch line
    const float glitchCycle = std::fmod(time * 0.4f, 6.0f);
    if (glitchCycle < 0.35f) {
        const float gy = std::fmod(time * 180.0f, fbH);
        const float gh = 3.0f + std::fmod(time * 37.0f, 6.0f);
        drawRect(0.0f, gy, fbW, gh, glm::vec4(0.85f, 0.82f, 0.60f, 0.08f));
    }

    endUI();
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

void Renderer::drawHud(int selectedSlot, const ItemSlot* hotbar, int slotCount,
                       float health, float maxHealth,
                       float hunger, float maxHunger,
                       float oxygen, float maxOxygen,
                       bool inWater, float hurtTimer, float animTime,
                       bool isCreative) {
    const float s = m_uiScale;

    beginUI();

    // 1. Crosshair (compact, high-contrast across all GUI scales)
    const float cx = static_cast<float>(m_fbWidth) * 0.5f;
    const float cy = static_cast<float>(m_fbHeight) * 0.5f;
    const float arm = std::max(3.0f, 2.5f * std::max(1.0f, s * 0.5f));
    const float thick = std::max(1.0f, s >= 3.0f ? 2.0f : 1.0f);
    const float outline = 1.0f;

    const glm::vec4 darkOutline(0.02f, 0.02f, 0.02f, 0.75f);
    const glm::vec4 innerColor(0.96f, 0.96f, 0.96f, 0.95f);

    drawRect(cx - arm - outline, cy - (thick * 0.5f) - outline, (arm * 2.0f) + (outline * 2.0f), thick + (outline * 2.0f), darkOutline);
    drawRect(cx - (thick * 0.5f) - outline, cy - arm - outline, thick + (outline * 2.0f), (arm * 2.0f) + (outline * 2.0f), darkOutline);
    drawRect(cx - arm, cy - thick * 0.5f, arm * 2.0f, thick, innerColor);
    drawRect(cx - thick * 0.5f, cy - arm, thick, arm * 2.0f, innerColor);

    // 2. 9-Slot Hotbar (Centered at bottom)
    const float slot = 20.0f * s;
    const float innerSlot = 16.0f * s;
    const float pad = (slot - innerSlot) * 0.5f;
    const float hotbarMarginY = 6.0f * s;
    const int count = std::min(slotCount, 9);
    const float totalHotbarW = count * slot;
    const float hotbarStartX = cx - totalHotbarW * 0.5f;

    // Hotbar container background frame
    drawRect(hotbarStartX - 1.0f * s, hotbarMarginY - 1.0f * s, totalHotbarW + 2.0f * s, slot + 2.0f * s, glm::vec4(0.0f, 0.0f, 0.0f, 0.60f));
    drawRect(hotbarStartX, hotbarMarginY, totalHotbarW, slot, glm::vec4(0.78f, 0.78f, 0.78f, 0.95f));

    for (int i = 0; i < count; ++i) {
        const float x = hotbarStartX + i * slot;
        const float y = hotbarMarginY;

        // Sunken slot bevel
        drawRect(x + 1.0f * s, y + 1.0f * s, slot - 2.0f * s, slot - 2.0f * s, glm::vec4(0.55f, 0.55f, 0.55f, 1.0f));
        drawRect(x + 1.0f * s, y + slot - 2.0f * s, slot - 2.0f * s, 1.0f * s, glm::vec4(0.22f, 0.22f, 0.22f, 1.0f));
        drawRect(x + 1.0f * s, y + 1.0f * s, 1.0f * s, slot - 2.0f * s, glm::vec4(0.22f, 0.22f, 0.22f, 1.0f));
        drawRect(x + slot - 2.0f * s, y + 1.0f * s, 1.0f * s, slot - 2.0f * s, glm::vec4(1.0f, 1.0f, 1.0f, 0.8f));
        drawRect(x + 1.0f * s, y + 1.0f * s, slot - 2.0f * s, 1.0f * s, glm::vec4(1.0f, 1.0f, 1.0f, 0.8f));

        if (!hotbar[i].empty()) {
            drawBlockIcon(x + pad, y + pad, innerSlot, innerSlot, hotbar[i].id);

            // Stack count
            if (hotbar[i].count > 1) {
                const std::string cntStr = std::to_string(hotbar[i].count);
                const float cntScale = std::max(1.0f, s * 0.65f);
                const float cw = textWidth(cntStr, cntScale);
                drawText(x + slot - cw - 2.0f * s + 1.0f, y + textHeight(cntScale) + 2.0f * s - 1.0f,
                         cntStr, cntScale, glm::vec4(0.0f, 0.0f, 0.0f, 0.85f));
                drawText(x + slot - cw - 2.0f * s, y + textHeight(cntScale) + 2.0f * s,
                         cntStr, cntScale, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            }

            // Durability bar for tools
            if (isTool(hotbar[i].id) && hotbar[i].durability > 0) {
                const float barH = std::max(1.5f, 1.5f * s);
                drawDurabilityBar(x + 2.0f * s, y + 2.0f * s, slot - 4.0f * s, barH,
                                  hotbar[i].durability, maxToolDurability(hotbar[i].id));
            }
        }

        // Active slot selection outline
        if (i == selectedSlot) {
            drawRect(x - 2.0f * s, y - 2.0f * s, slot + 4.0f * s, 2.0f * s, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            drawRect(x - 2.0f * s, y + slot, slot + 4.0f * s, 2.0f * s, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            drawRect(x - 2.0f * s, y - 2.0f * s, 2.0f * s, slot + 4.0f * s, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            drawRect(x + slot, y - 2.0f * s, 2.0f * s, slot + 4.0f * s, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
    }

    // 3. Survival Vitals (Health Hearts & Hunger Drumsticks above hotbar)
    if (!isCreative) {
        const float iconSize = 9.0f * s;
        const float iconSpacing = 8.0f * s;
        const float vitalsY = hotbarMarginY + slot + 6.0f * s;

        // XP Bar (Directly between hotbar and hearts)
        const float xpBarW = totalHotbarW;
        const float xpBarH = 3.0f * s;
        const float xpBarY = hotbarMarginY + slot + 2.0f * s;
        drawRect(hotbarStartX, xpBarY, xpBarW, xpBarH, glm::vec4(0.0f, 0.0f, 0.0f, 0.8f));
        drawRect(hotbarStartX + 1.0f * s, xpBarY + 0.5f * s, xpBarW - 2.0f * s, xpBarH - 1.0f * s, glm::vec4(0.1f, 0.25f, 0.05f, 1.0f));
        drawRect(hotbarStartX + 1.0f * s, xpBarY + 0.5f * s, (xpBarW - 2.0f * s) * 0.45f, xpBarH - 1.0f * s, glm::vec4(0.50f, 1.0f, 0.15f, 1.0f));

        // Green Level Number
        const std::string lvlStr = "1";
        const float lvlScale = std::max(1.0f, s * 0.70f);
        const float lw = textWidth(lvlStr, lvlScale);
        const float lh = textHeight(lvlScale);
        drawText(cx - lw * 0.5f + 1.0f, xpBarY + lh + 1.0f * s - 1.0f, lvlStr, lvlScale, glm::vec4(0.0f, 0.0f, 0.0f, 0.9f));
        drawText(cx - lw * 0.5f, xpBarY + lh + 1.0f * s, lvlStr, lvlScale, glm::vec4(0.50f, 1.0f, 0.15f, 1.0f));

        // 10 Health Hearts on Left (0..20 HP, 2 HP per heart)
        const float heartsStartX = hotbarStartX;
        const float hp = std::clamp(health, 0.0f, maxHealth);
        const int hpPoints = static_cast<int>(std::ceil(hp * (20.0f / maxHealth)));

        for (int h = 0; h < 10; ++h) {
            float hx = heartsStartX + h * iconSpacing;
            float hy = vitalsY;

            // Low health jitter / damage shake
            if (hpPoints <= 4) {
                const float shake = std::sin(animTime * 20.0f + h) * 1.5f * s;
                hy += shake;
            } else if (hurtTimer > 0.0f) {
                hy += (std::rand() % 3 - 1) * 1.0f * s;
            }

            drawTexturedRect(hx, hy, iconSize, iconSize, TextureTile::HeartEmpty);

            const int heartVal = (h + 1) * 2;
            if (hpPoints >= heartVal) {
                drawTexturedRect(hx, hy, iconSize, iconSize, TextureTile::HeartFull);
            } else if (hpPoints == heartVal - 1) {
                drawTexturedRect(hx, hy, iconSize, iconSize, TextureTile::HeartHalf);
            }
        }

        // 10 Food Drumsticks on Right (0..20 Hunger, 2 Hunger per drumstick)
        const float foodStartX = hotbarStartX + totalHotbarW - iconSize;
        const float hng = std::clamp(hunger, 0.0f, maxHunger);
        const int hngPoints = static_cast<int>(std::ceil(hng * (20.0f / maxHunger)));

        for (int f = 0; f < 10; ++f) {
            const float fx = foodStartX - f * iconSpacing;
            const float fy = vitalsY;

            drawTexturedRect(fx, fy, iconSize, iconSize, TextureTile::FoodEmpty);

            const int foodVal = (f + 1) * 2;
            if (hngPoints >= foodVal) {
                drawTexturedRect(fx, fy, iconSize, iconSize, TextureTile::FoodFull);
            } else if (hngPoints == foodVal - 1) {
                drawTexturedRect(fx, fy, iconSize, iconSize, TextureTile::FoodHalf);
            }
        }

        // 10 Air Bubbles above Food when in water
        if (inWater || oxygen < maxOxygen) {
            const float oxyRatio = std::clamp(oxygen / maxOxygen, 0.0f, 1.0f);
            const int bubbles = static_cast<int>(std::ceil(oxyRatio * 10.0f));
            const float bubbleY = vitalsY + iconSize + 2.0f * s;

            for (int b = 0; b < bubbles; ++b) {
                const float bx = foodStartX - b * iconSpacing;
                drawTexturedRect(bx, bubbleY, iconSize, iconSize, TextureTile::AirBubble);
            }
        }
    }

    endUI();
}

void Renderer::drawHurtVignette(float hurtTimer, float healthRatio, float animTime) {
    float flashAlpha = 0.0f;
    if (hurtTimer > 0.0f) {
        flashAlpha = (hurtTimer / 0.45f) * 0.50f;
    }

    float dangerAlpha = 0.0f;
    if (healthRatio <= 0.40f) {
        const float pulse = 0.6f + 0.4f * std::sin(animTime * 6.0f);
        dangerAlpha = (1.0f - (healthRatio / 0.40f)) * 0.40f * pulse;
    }

    const float totalAlpha = std::clamp(flashAlpha + dangerAlpha, 0.0f, 0.75f);
    if (totalAlpha <= 0.01f) return;

    beginUI();

    const float fbW = static_cast<float>(m_fbWidth);
    const float fbH = static_cast<float>(m_fbHeight);
    const float thick = std::min(fbW, fbH) * 0.16f;

    const glm::vec4 colSolid(0.60f, 0.02f, 0.04f, totalAlpha);
    const glm::vec4 colTrans(0.60f, 0.02f, 0.04f, 0.0f);

    drawGradientRect(0.0f, 0.0f, fbW, thick, colSolid, colTrans);
    drawGradientRect(0.0f, fbH - thick, fbW, thick, colTrans, colSolid);
    drawRect(0.0f, 0.0f, thick * 0.4f, fbH, colSolid * 0.4f);
    drawRect(fbW - thick * 0.4f, 0.0f, thick * 0.4f, fbH, colSolid * 0.4f);

    endUI();
}

void Renderer::drawTitleBanner(const std::string& title, const std::string& subtitle, float alpha, float /*animTime*/) {
    if (alpha <= 0.01f || title.empty()) return;

    const float s = m_uiScale;
    beginUI();

    const float fbW = static_cast<float>(m_fbWidth);
    const float fbH = static_cast<float>(m_fbHeight);
    const float centerY = fbH * 0.72f;

    // Font metrics and scaling
    const float titleScale = std::max(2.0f, s * 1.5f);
    const float titleW = textWidth(title, titleScale);
    const float titleH = textHeight(titleScale);

    const float subScale = std::max(1.0f, s * 0.75f);
    const float subW = subtitle.empty() ? 0.0f : textWidth(subtitle, subScale);
    const float subH = subtitle.empty() ? 0.0f : textHeight(subScale);

    const float gap = subtitle.empty() ? 0.0f : (6.0f * s);
    const float totalContentH = titleH + gap + subH;
    const float contentTop = centerY + totalContentH * 0.5f;
    const float contentBottom = centerY - totalContentH * 0.5f;

    const float padY = 8.0f * s;
    const float topLineY = contentTop + padY;
    const float bottomLineY = contentBottom - padY;

    // 1. Decorative horizontal separator lines
    const float maxTextW = std::max(titleW, subW);
    const float lineW = std::min(fbW * 0.75f, std::max(260.0f * s, maxTextW + 60.0f * s));
    const float lineX = (fbW - lineW) * 0.5f;
    const float lineThick = std::max(1.0f, 1.5f * s);

    // Top gold line & diamond
    drawRect(lineX, topLineY, lineW, lineThick, glm::vec4(0.92f, 0.82f, 0.35f, alpha * 0.85f));
    const float dSize = 4.0f * s;
    drawRect((fbW - dSize) * 0.5f, topLineY - (dSize * 0.5f) + (lineThick * 0.5f), dSize, dSize, glm::vec4(1.0f, 0.95f, 0.45f, alpha));

    // Bottom gold line & diamond
    drawRect(lineX, bottomLineY, lineW, lineThick, glm::vec4(0.92f, 0.82f, 0.35f, alpha * 0.85f));
    drawRect((fbW - dSize) * 0.5f, bottomLineY - (dSize * 0.5f) + (lineThick * 0.5f), dSize, dSize, glm::vec4(1.0f, 0.95f, 0.45f, alpha));

    // 2. Large Title Text (centered horizontally and positioned at contentTop)
    const float titleX = (fbW - titleW) * 0.5f;
    const float titleY = contentTop;

    // Shadow
    drawText(titleX + 2.0f * s, titleY - 2.0f * s, title, titleScale, glm::vec4(0.0f, 0.0f, 0.0f, alpha * 0.95f));
    // Foreground Title
    drawText(titleX, titleY, title, titleScale, glm::vec4(1.0f, 0.92f, 0.35f, alpha));

    // 3. Subtitle Text (centered horizontally and positioned below Title)
    if (!subtitle.empty()) {
        const float subX = (fbW - subW) * 0.5f;
        const float subY = titleY - titleH - gap;

        // Subtitle Shadow
        drawText(subX + 1.0f * s, subY - 1.0f * s, subtitle, subScale, glm::vec4(0.0f, 0.0f, 0.0f, alpha * 0.90f));
        // Subtitle Foreground (Parchment/Amber)
        drawText(subX, subY, subtitle, subScale, glm::vec4(0.95f, 0.85f, 0.75f, alpha * 0.95f));
    }

    endUI();
}

void Renderer::drawDeathScreen(float /*animTime*/, const glm::vec2& mousePos,
                              bool& outHoverRespawn, bool& outHoverQuit) {
    const float s = m_uiScale;
    beginUI();

    const float fbW = static_cast<float>(m_fbWidth);
    const float fbH = static_cast<float>(m_fbHeight);

    // Dark crimson shroud
    drawRect(0.0f, 0.0f, fbW, fbH, glm::vec4(0.06f, 0.01f, 0.02f, 0.88f));

    // Death Title "YOU DIED"
    const std::string title = "YOU DIED";
    const float titleScale = std::max(2.5f, s * 1.5f);
    const float titleW = textWidth(title, titleScale);
    const float titleX = (fbW - titleW) * 0.5f;
    const float titleY = fbH * 0.68f;

    drawText(titleX + 2.0f, titleY - 2.0f, title, titleScale, glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
    drawText(titleX, titleY, title, titleScale, glm::vec4(0.95f, 0.08f, 0.10f, 1.0f));

    // Subtitle
    const std::string subtitle = "The darkness consumed your mortal soul...";
    const float subScale = std::max(1.0f, s * 0.65f);
    const float subW = textWidth(subtitle, subScale);
    const float subX = (fbW - subW) * 0.5f;
    const float subY = titleY - (22.0f * s);
    drawText(subX, subY, subtitle, subScale, glm::vec4(0.75f, 0.65f, 0.65f, 0.85f));

    // Buttons
    const float btnW = 120.0f * s;
    const float btnH = 18.0f * s;
    const float btnX = (fbW - btnW) * 0.5f;
    const float respawnY = fbH * 0.42f;
    const float quitY = respawnY - (26.0f * s);

    // Respawn button
    outHoverRespawn = (mousePos.x >= btnX && mousePos.x <= btnX + btnW &&
                       mousePos.y >= respawnY && mousePos.y <= respawnY + btnH);
    const glm::vec4 respawnBg = outHoverRespawn ? glm::vec4(0.40f, 0.12f, 0.15f, 0.95f) : glm::vec4(0.15f, 0.08f, 0.10f, 0.90f);
    const glm::vec4 respawnBorder = outHoverRespawn ? glm::vec4(0.95f, 0.25f, 0.30f, 1.0f) : glm::vec4(0.35f, 0.20f, 0.22f, 0.9f);
    drawRect(btnX, respawnY, btnW, btnH, respawnBg);
    drawRect(btnX, respawnY, btnW, 1.0f * s, respawnBorder);
    drawRect(btnX, respawnY + btnH - 1.0f * s, btnW, 1.0f * s, respawnBorder);
    drawRect(btnX, respawnY, 1.0f * s, btnH, respawnBorder);
    drawRect(btnX + btnW - 1.0f * s, respawnY, 1.0f * s, btnH, respawnBorder);

    const std::string respawnText = "RESPAWN";
    const float btnTextScale = std::max(1.0f, s * 0.70f);
    const float rw = textWidth(respawnText, btnTextScale);
    const float rh = textHeight(btnTextScale);
    drawText(btnX + (btnW - rw) * 0.5f, respawnY + (btnH + rh) * 0.5f, respawnText, btnTextScale,
             outHoverRespawn ? glm::vec4(1.0f, 0.95f, 0.95f, 1.0f) : glm::vec4(0.85f, 0.75f, 0.75f, 0.9f));

    // Title menu button
    outHoverQuit = (mousePos.x >= btnX && mousePos.x <= btnX + btnW &&
                    mousePos.y >= quitY && mousePos.y <= quitY + btnH);
    const glm::vec4 quitBg = outHoverQuit ? glm::vec4(0.30f, 0.12f, 0.15f, 0.95f) : glm::vec4(0.12f, 0.08f, 0.10f, 0.90f);
    const glm::vec4 quitBorder = outHoverQuit ? glm::vec4(0.85f, 0.25f, 0.30f, 1.0f) : glm::vec4(0.30f, 0.18f, 0.20f, 0.9f);
    drawRect(btnX, quitY, btnW, btnH, quitBg);
    drawRect(btnX, quitY, btnW, 1.0f * s, quitBorder);
    drawRect(btnX, quitY + btnH - 1.0f * s, btnW, 1.0f * s, quitBorder);
    drawRect(btnX, quitY, 1.0f * s, btnH, quitBorder);
    drawRect(btnX + btnW - 1.0f * s, quitY, 1.0f * s, btnH, quitBorder);

    const std::string quitText = "TITLE MENU";
    const float qw = textWidth(quitText, btnTextScale);
    const float qh = textHeight(btnTextScale);
    drawText(btnX + (btnW - qw) * 0.5f, quitY + (btnH + qh) * 0.5f, quitText, btnTextScale,
             outHoverQuit ? glm::vec4(1.0f, 0.95f, 0.95f, 1.0f) : glm::vec4(0.85f, 0.75f, 0.75f, 0.9f));

    endUI();
}

void Renderer::drawChat(const std::vector<ChatMessage>& messages,
                        bool chatOpen,
                        const std::string& currentInput,
                        int cursorIndex,
                        const std::vector<std::string>& suggestions,
                        int selectedSuggestion,
                        float uiTime) {
    const float s = m_uiScale;
    const float fontScale = std::max(1.0f, s * 0.70f);
    const float fontH = textHeight(fontScale);
    const float marginX = 8.0f * s;
    const float chatBottom = 34.0f * s; // Positioned above the hotbar
    const float maxChatWidth = std::min(480.0f * s, static_cast<float>(m_fbWidth) - 2.0f * marginX);

    beginUI();

    const float inputH = fontH + 8.0f * s;
    const float inputY = chatBottom;
    const float inputW = maxChatWidth;

    // 1. Draw Chat Messages Log
    const int maxVisible = chatOpen ? 10 : 8;
    int drawn = 0;

    const float msgBaseY = chatOpen ? (inputY + inputH + 4.0f * s) : chatBottom;
    const float msgRowH = fontH + 5.0f * s;

    for (int i = static_cast<int>(messages.size()) - 1; i >= 0 && drawn < maxVisible; --i) {
        const auto& msg = messages[static_cast<size_t>(i)];
        float alpha = 1.0f;
        if (!chatOpen) {
            if (msg.timeRemaining <= 0.0f) continue;
            if (msg.timeRemaining < 2.0f) {
                alpha = msg.timeRemaining / 2.0f;
            }
        }

        const float msgBottom = msgBaseY + static_cast<float>(drawn) * msgRowH;
        const float msgBoxH = fontH + 3.0f * s;
        const float msgTextTopY = msgBottom + (msgBoxH + fontH) * 0.5f;
        const float textW = textWidth(msg.text, fontScale);

        // Dark background plate behind text for legibility
        drawRect(marginX - 3.0f * s, msgBottom, textW + 6.0f * s, msgBoxH,
                 glm::vec4(0.0f, 0.0f, 0.0f, (chatOpen ? 0.65f : 0.45f) * alpha));

        // Text shadow & text
        drawText(marginX + 1.0f * s, msgTextTopY - 1.0f * s, msg.text, fontScale,
                 glm::vec4(0.0f, 0.0f, 0.0f, 0.8f * alpha));
        drawText(marginX, msgTextTopY, msg.text, fontScale,
                 glm::vec4(msg.color.r, msg.color.g, msg.color.b, msg.color.a * alpha));

        drawn++;
    }

    // 2. If chat is open: Draw Input Bar & Autocomplete Suggestion Box
    if (chatOpen) {
        // Input bar outer borders and background
        drawRect(marginX - 2.0f * s, inputY - 2.0f * s, inputW + 4.0f * s, inputH + 4.0f * s,
                 glm::vec4(0.0f, 0.0f, 0.0f, 0.85f));
        drawRect(marginX - 1.0f * s, inputY - 1.0f * s, inputW + 2.0f * s, inputH + 2.0f * s,
                 glm::vec4(0.35f, 0.35f, 0.40f, 0.9f));
        drawRect(marginX, inputY, inputW, inputH, glm::vec4(0.08f, 0.08f, 0.10f, 0.95f));

        // Prompt symbol "> "
        const std::string prompt = "> ";
        const float promptW = textWidth(prompt, fontScale);
        const float textTopY = inputY + (inputH + fontH) * 0.5f;

        drawText(marginX + 4.0f * s, textTopY, prompt, fontScale,
                 glm::vec4(0.40f, 0.85f, 1.0f, 1.0f));

        // Input text
        const float textX = marginX + 4.0f * s + promptW;
        drawText(textX, textTopY, currentInput, fontScale, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        // Blinking cursor
        const bool cursorBlink = std::fmod(uiTime, 0.8f) < 0.45f;
        if (cursorBlink) {
            std::string sub = currentInput.substr(0, static_cast<size_t>(std::clamp(cursorIndex, 0, static_cast<int>(currentInput.size()))));
            float cursorOffset = textWidth(sub, fontScale);
            drawRect(textX + cursorOffset, textTopY - fontH, 2.0f * s, fontH, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }

        // 3. Autocomplete Suggestions Box
        if (!suggestions.empty()) {
            const int maxSug = std::min(8, static_cast<int>(suggestions.size()));
            float sugWidest = 80.0f * s;
            for (int i = 0; i < maxSug; ++i) {
                sugWidest = std::max(sugWidest, textWidth(suggestions[static_cast<size_t>(i)], fontScale));
            }
            const float sugW = sugWidest + 16.0f * s;
            const float sugRowH = fontH + 5.0f * s;
            const float sugH = static_cast<float>(maxSug) * sugRowH + 4.0f * s;
            const float sugX = marginX;
            const float sugY = inputY + inputH + 4.0f * s;

            // Box shadow & border
            drawRect(sugX - 2.0f * s, sugY - 2.0f * s, sugW + 4.0f * s, sugH + 4.0f * s,
                     glm::vec4(0.0f, 0.0f, 0.0f, 0.8f));
            drawRect(sugX - 1.0f * s, sugY - 1.0f * s, sugW + 2.0f * s, sugH + 2.0f * s,
                     glm::vec4(0.4f, 0.4f, 0.5f, 0.9f));
            drawRect(sugX, sugY, sugW, sugH, glm::vec4(0.08f, 0.09f, 0.12f, 0.95f));

            for (int i = 0; i < maxSug; ++i) {
                const float rowY = sugY + sugH - static_cast<float>(i + 1) * sugRowH - 2.0f * s;
                const bool isSel = (selectedSuggestion == i);

                if (isSel) {
                    drawRect(sugX + 2.0f * s, rowY, sugW - 4.0f * s, sugRowH,
                             glm::vec4(0.25f, 0.45f, 0.75f, 0.85f));
                }

                const float sugTextTopY = rowY + (sugRowH + fontH) * 0.5f;
                drawText(sugX + 6.0f * s, sugTextTopY, suggestions[static_cast<size_t>(i)], fontScale,
                         isSel ? glm::vec4(1.0f, 1.0f, 0.3f, 1.0f) : glm::vec4(0.85f, 0.85f, 0.85f, 1.0f));
            }
        }
    }

    endUI();
}

void Renderer::drawDebugOverlay(const std::vector<std::string>& lines) {
    if (lines.empty()) return;

    const int savedDrawCalls = m_stats.drawCalls;
    const int savedUiCalls = m_stats.uiDrawCalls;
    const uint32_t savedTriangles = m_stats.triangles;

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
        drawText(x + pad + 1.0f, textY - 1.0f, line, scale, glm::vec4(0.0f, 0.0f, 0.0f, 0.75f));
        drawText(x + pad, textY, line, scale, glm::vec4(1.0f, 1.0f, 1.0f, 0.95f));
        textY -= lineHeight;
    }
    endUI();

    m_stats.drawCalls = savedDrawCalls;
    m_stats.uiDrawCalls = savedUiCalls;
    m_stats.triangles = savedTriangles;
}

// ---------------------------------------------------------------------------
// Modern Minecraft Container Drawing Helpers
// ---------------------------------------------------------------------------
namespace {

void drawMinecraftContainer(Renderer* ren, float x, float y, float w, float h, float s) {
    // Outer black border
    ren->drawRect(x - 2.0f * s, y - 2.0f * s, w + 4.0f * s, h + 4.0f * s, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    // White/highlight top & left edge
    ren->drawRect(x - 1.0f * s, y, w + 2.0f * s, h + 1.0f * s, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    // Dark gray bottom & right edge
    ren->drawRect(x, y - 1.0f * s, w + 1.0f * s, 1.0f * s, glm::vec4(0.33f, 0.33f, 0.33f, 1.0f));
    ren->drawRect(x + w, y, 1.0f * s, h, glm::vec4(0.33f, 0.33f, 0.33f, 1.0f));
    // Container base light gray fill
    ren->drawRect(x, y, w, h, glm::vec4(0.776f, 0.776f, 0.776f, 1.0f));
}

void drawMinecraftSlot(Renderer* ren, float x, float y, float slotSize, const ItemSlot& item,
                       const glm::vec2& mousePos, float s, std::string& outTooltip,
                       TextureTile silhouette = TextureTile::Count, bool isSelected = false) {
    const float border = 1.0f * s;
    const bool hovered = (mousePos.x >= x && mousePos.x <= x + slotSize &&
                          mousePos.y >= y && mousePos.y <= y + slotSize);

    // Sunken recess
    ren->drawRect(x, y, slotSize, slotSize, glm::vec4(0.22f, 0.22f, 0.22f, 1.0f));
    ren->drawRect(x + border, y, slotSize - border, slotSize - border, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    ren->drawRect(x + border, y + border, slotSize - 2.0f * border, slotSize - 2.0f * border, glm::vec4(0.55f, 0.55f, 0.55f, 1.0f));

    if (isSelected) {
        ren->drawRect(x - 1.0f * s, y - 1.0f * s, slotSize + 2.0f * s, slotSize + 2.0f * s, glm::vec4(1.0f, 1.0f, 1.0f, 0.9f));
    }

    if (item.empty()) {
        if (silhouette != TextureTile::Count) {
            ren->drawTexturedRect(x + 1.5f * s, y + 1.5f * s, slotSize - 3.0f * s, slotSize - 3.0f * s, silhouette);
        }
    } else {
        const float iconPad = 1.5f * s;
        ren->drawBlockIcon(x + iconPad, y + iconPad, slotSize - 2.0f * iconPad, slotSize - 2.0f * iconPad, item.id);

        if (item.count > 1) {
            const std::string cntStr = std::to_string(item.count);
            const float cntScale = std::max(1.0f, s * 0.55f);
            const float cw = ren->textWidth(cntStr, cntScale);
            ren->drawText(x + slotSize - cw - 1.0f * s + 1.0f, y + ren->textHeight(cntScale) + 1.0f * s - 1.0f,
                          cntStr, cntScale, glm::vec4(0.0f, 0.0f, 0.0f, 0.85f));
            ren->drawText(x + slotSize - cw - 1.0f * s, y + ren->textHeight(cntScale) + 1.0f * s,
                          cntStr, cntScale, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }

        if (isTool(item.id) && item.durability > 0) {
            const float barH = std::max(1.5f, 1.2f * s);
            ren->drawDurabilityBar(x + border, y + border, slotSize - 2.0f * border, barH,
                                  item.durability, maxToolDurability(item.id));
        }
    }

    if (hovered) {
        ren->drawRect(x + border, y + border, slotSize - 2.0f * border, slotSize - 2.0f * border,
                      glm::vec4(1.0f, 1.0f, 1.0f, 0.30f));
        if (!item.empty()) {
            outTooltip = blockDef(item.id).name;
            if (isTool(item.id)) {
                outTooltip += " [" + std::to_string(item.durability) + "/" +
                              std::to_string(maxToolDurability(item.id)) + "]";
            }
        }
    }
}

void drawInventoryGrid(Renderer* ren, float startX, float startY, float slotSize, float gap,
                       const ItemSlot* inventory, int invCount, const ItemSlot* hotbar, int hotbarCount,
                       int selectedHotbarSlot, const glm::vec2& mousePos, float s, std::string& outTooltip) {
    // 3 Rows x 9 Columns Main Inventory (27 slots)
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 9; ++col) {
            const int idx = row * 9 + col;
            const float sx = startX + col * (slotSize + gap);
            const float sy = startY - (row + 1) * slotSize - row * gap;
            const ItemSlot& item = (idx < invCount) ? inventory[idx] : ItemSlot();
            drawMinecraftSlot(ren, sx, sy, slotSize, item, mousePos, s, outTooltip);
        }
    }

    // 1 Row x 9 Columns Hotbar (9 slots)
    const float hotbarY = startY - 3 * (slotSize + gap) - 4.0f * s - slotSize;
    for (int col = 0; col < 9 && col < hotbarCount; ++col) {
        const float sx = startX + col * (slotSize + gap);
        drawMinecraftSlot(ren, sx, hotbarY, slotSize, hotbar[col], mousePos, s, outTooltip,
                          TextureTile::Count, (col == selectedHotbarSlot));
    }
}

void renderTooltipAndCursor(Renderer* ren, const std::string& tooltip, const ItemSlot& heldItem,
                            const glm::vec2& mousePos, float s, int fbWidth) {
    if (!tooltip.empty()) {
        const float tipScale = std::max(1.0f, s * 0.70f);
        const float tipW = ren->textWidth(tooltip, tipScale);
        const float tipH = ren->textHeight(tipScale);
        const float tipX = std::min(mousePos.x + 10.0f * s, static_cast<float>(fbWidth) - tipW - 10.0f * s);
        const float tipY = std::max(mousePos.y + tipH + 4.0f * s, tipH + 10.0f * s);

        ren->drawRect(tipX - 4.0f * s, tipY - tipH - 4.0f * s, tipW + 8.0f * s, tipH + 8.0f * s, glm::vec4(0.06f, 0.02f, 0.10f, 0.96f));
        ren->drawRect(tipX - 3.0f * s, tipY - tipH - 3.0f * s, tipW + 6.0f * s, tipH + 6.0f * s, glm::vec4(0.20f, 0.05f, 0.50f, 0.85f));
        ren->drawRect(tipX - 2.0f * s, tipY - tipH - 2.0f * s, tipW + 4.0f * s, tipH + 4.0f * s, glm::vec4(0.06f, 0.02f, 0.10f, 0.96f));
        ren->drawText(tipX, tipY, tooltip, tipScale, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    if (!heldItem.empty()) {
        const float itemSize = 16.0f * s;
        ren->drawRect(mousePos.x - itemSize * 0.5f + 1.5f * s, mousePos.y - itemSize * 0.5f - 1.5f * s,
                      itemSize, itemSize, glm::vec4(0.0f, 0.0f, 0.0f, 0.4f));
        ren->drawBlockIcon(mousePos.x - itemSize * 0.5f, mousePos.y - itemSize * 0.5f,
                           itemSize, itemSize, heldItem.id);
        if (heldItem.count > 1) {
            const std::string cntStr = std::to_string(heldItem.count);
            const float cntScale = std::max(1.0f, s * 0.60f);
            ren->drawText(mousePos.x + itemSize * 0.3f, mousePos.y - itemSize * 0.3f,
                          cntStr, cntScale, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
    }
}

} // namespace

void Renderer::drawSurvivalInventory(int selectedHotbarSlot,
                                     const ItemSlot* hotbar, int hotbarCount,
                                     const ItemSlot* inventory, int invCount,
                                     const ItemSlot* armor, int armorCount,
                                     const ItemSlot& offhand,
                                     const ItemSlot* craftingSlots,
                                     const ItemSlot& craftingResult,
                                     const ItemSlot& heldItem,
                                     const glm::vec2& mousePos,
                                     bool recipeBookOpen) {
    const float s = m_uiScale;
    const float containerW = 176.0f * s;
    const float containerH = 166.0f * s;
    const float cx = static_cast<float>(m_fbWidth) * 0.5f;
    const float cy = static_cast<float>(m_fbHeight) * 0.5f;
    const float left = cx - containerW * 0.5f;
    const float bottom = cy - containerH * 0.5f;

    const float slotSize = 18.0f * s;
    const float gap = 0.0f; // Authentic contiguous slots with 1px border

    beginUI();

    // Dim background
    drawRect(0.0f, 0.0f, static_cast<float>(m_fbWidth), static_cast<float>(m_fbHeight), glm::vec4(0.0f, 0.0f, 0.0f, 0.55f));

    // Main Minecraft container window
    drawMinecraftContainer(this, left, bottom, containerW, containerH, s);

    std::string tooltip;

    // 1. Top-Left: 4 Armor Slots (Helmet, Chestplate, Leggings, Boots)
    const float armorX = left + 8.0f * s;
    const float armorTop = bottom + containerH - 8.0f * s;
    const TextureTile armorSilhouettes[4] = {
        TextureTile::HelmetIcon, TextureTile::ChestplateIcon,
        TextureTile::LeggingsIcon, TextureTile::BootsIcon
    };

    for (int a = 0; a < 4; ++a) {
        const float ay = armorTop - (a + 1) * slotSize;
        const ItemSlot& item = (a < armorCount) ? armor[a] : ItemSlot();
        drawMinecraftSlot(this, armorX, ay, slotSize, item, mousePos, s, tooltip, armorSilhouettes[a]);
    }

    // 2. Center: Player 2D/3D Avatar Preview Box
    const float playerBoxX = armorX + slotSize + 2.0f * s;
    const float playerBoxY = armorTop - 4 * slotSize;
    const float playerBoxW = 51.0f * s;
    const float playerBoxH = 72.0f * s;

    // Sunken dark box
    drawRect(playerBoxX, playerBoxY, playerBoxW, playerBoxH, glm::vec4(0.22f, 0.22f, 0.22f, 1.0f));
    drawRect(playerBoxX + 1.0f * s, playerBoxY, playerBoxW - 1.0f * s, playerBoxH - 1.0f * s, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    drawRect(playerBoxX + 1.0f * s, playerBoxY + 1.0f * s, playerBoxW - 2.0f * s, playerBoxH - 2.0f * s, glm::vec4(0.05f, 0.05f, 0.06f, 1.0f));

    // Render 2D Player Face & Torso Preview in box
    const float faceSize = 24.0f * s;
    drawTexturedRect(playerBoxX + (playerBoxW - faceSize) * 0.5f, playerBoxY + playerBoxH - faceSize - 8.0f * s,
                     faceSize, faceSize, TextureTile::PlayerFace);
    const float torsoW = 24.0f * s;
    const float torsoH = 28.0f * s;
    drawTexturedRect(playerBoxX + (playerBoxW - torsoW) * 0.5f, playerBoxY + 10.0f * s,
                     torsoW, torsoH, TextureTile::PlayerTorso);

    // 3. Offhand slot (Right of player box)
    const float offhandX = playerBoxX + playerBoxW + 2.0f * s;
    const float offhandY = playerBoxY;
    drawMinecraftSlot(this, offhandX, offhandY, slotSize, offhand, mousePos, s, tooltip, TextureTile::OffhandIcon);

    // 4. Top-Right: "Crafting" label + 2x2 Crafting Grid + Recipe Book button + Arrow + Result Slot
    const float craftingGridX = left + 98.0f * s;
    const float craftingGridTop = armorTop - 10.0f * s;

    const float titleScale = std::max(1.0f, s * 0.65f);
    drawText(craftingGridX, armorTop - 2.0f * s, "Crafting", titleScale, glm::vec4(0.25f, 0.25f, 0.25f, 1.0f));

    // Green Recipe Book Button
    const float bookBtnX = craftingGridX + 2 * slotSize + 4.0f * s;
    const float bookBtnY = armorTop - 8.0f * s;
    const float bookBtnSize = 14.0f * s;
    const bool bookHovered = (mousePos.x >= bookBtnX && mousePos.x <= bookBtnX + bookBtnSize &&
                              mousePos.y >= bookBtnY && mousePos.y <= bookBtnY + bookBtnSize);
    drawRect(bookBtnX, bookBtnY, bookBtnSize, bookBtnSize,
             recipeBookOpen ? glm::vec4(0.2f, 0.7f, 0.25f, 1.0f) :
             (bookHovered ? glm::vec4(0.35f, 0.85f, 0.35f, 1.0f) : glm::vec4(0.18f, 0.55f, 0.20f, 1.0f)));
    drawTexturedRect(bookBtnX + 1.0f * s, bookBtnY + 1.0f * s, bookBtnSize - 2.0f * s, bookBtnSize - 2.0f * s, TextureTile::RecipeBook);
    if (bookHovered) tooltip = "Recipe Book (Toggle)";

    // 2x2 Crafting Slots (indices 0..3)
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            const int idx = r * 2 + c;
            const float sx = craftingGridX + c * slotSize;
            const float sy = craftingGridTop - (r + 1) * slotSize;
            const ItemSlot& item = (craftingSlots != nullptr) ? craftingSlots[idx] : ItemSlot();
            drawMinecraftSlot(this, sx, sy, slotSize, item, mousePos, s, tooltip);
        }
    }

    // Crafting Arrow -->
    const float arrowX = craftingGridX + 2 * slotSize + 6.0f * s;
    const float arrowY = craftingGridTop - 1.5f * slotSize;
    drawTexturedRect(arrowX, arrowY, 16.0f * s, 16.0f * s, TextureTile::CraftingArrow);

    // 1 Result Slot
    const float resultX = arrowX + 22.0f * s;
    const float resultY = craftingGridTop - 1.7f * slotSize;
    const float resSlotSize = 22.0f * s;
    drawMinecraftSlot(this, resultX, resultY, resSlotSize, craftingResult, mousePos, s, tooltip);

    // 5. Bottom: "Inventory" Label + 3x9 Main Grid + 1x9 Hotbar Grid
    const float invStartX = left + 8.0f * s;
    const float invStartY = bottom + 74.0f * s;
    drawText(invStartX, invStartY + 10.0f * s, "Inventory", titleScale, glm::vec4(0.25f, 0.25f, 0.25f, 1.0f));

    drawInventoryGrid(this, invStartX, invStartY, slotSize, gap,
                      inventory, invCount, hotbar, hotbarCount, selectedHotbarSlot, mousePos, s, tooltip);

    // Render hover tooltip and dragged item on top
    renderTooltipAndCursor(this, tooltip, heldItem, mousePos, s, m_fbWidth);

    endUI();
}

void Renderer::drawCraftingTableWorkbench(int selectedHotbarSlot,
                                         const ItemSlot* hotbar, int hotbarCount,
                                         const ItemSlot* inventory, int invCount,
                                         const ItemSlot* craftingSlots,
                                         const ItemSlot& craftingResult,
                                         const ItemSlot& heldItem,
                                         const glm::vec2& mousePos,
                                         bool recipeBookOpen) {
    const float s = m_uiScale;
    const float containerW = 176.0f * s;
    const float containerH = 166.0f * s;
    const float cx = static_cast<float>(m_fbWidth) * 0.5f;
    const float cy = static_cast<float>(m_fbHeight) * 0.5f;
    const float left = cx - containerW * 0.5f;
    const float bottom = cy - containerH * 0.5f;

    const float slotSize = 18.0f * s;
    const float gap = 0.0f;

    beginUI();

    // Dim background
    drawRect(0.0f, 0.0f, static_cast<float>(m_fbWidth), static_cast<float>(m_fbHeight), glm::vec4(0.0f, 0.0f, 0.0f, 0.55f));

    // Main container window
    drawMinecraftContainer(this, left, bottom, containerW, containerH, s);

    std::string tooltip;

    // 1. Header & Crafting label
    const float titleScale = std::max(1.0f, s * 0.65f);
    const float topY = bottom + containerH - 8.0f * s;
    drawText(left + 28.0f * s, topY - 2.0f * s, "Crafting", titleScale, glm::vec4(0.25f, 0.25f, 0.25f, 1.0f));

    // Green Recipe Book Button
    const float bookBtnX = left + 8.0f * s;
    const float bookBtnY = topY - 14.0f * s;
    const float bookBtnSize = 14.0f * s;
    const bool bookHovered = (mousePos.x >= bookBtnX && mousePos.x <= bookBtnX + bookBtnSize &&
                              mousePos.y >= bookBtnY && mousePos.y <= bookBtnY + bookBtnSize);
    drawRect(bookBtnX, bookBtnY, bookBtnSize, bookBtnSize,
             recipeBookOpen ? glm::vec4(0.2f, 0.7f, 0.25f, 1.0f) :
             (bookHovered ? glm::vec4(0.35f, 0.85f, 0.35f, 1.0f) : glm::vec4(0.18f, 0.55f, 0.20f, 1.0f)));
    drawTexturedRect(bookBtnX + 1.0f * s, bookBtnY + 1.0f * s, bookBtnSize - 2.0f * s, bookBtnSize - 2.0f * s, TextureTile::RecipeBook);
    if (bookHovered) tooltip = "Recipe Book (Toggle)";

    // 2. 3x3 Crafting Grid (indices 0..8)
    const float grid3x3StartX = left + 30.0f * s;
    const float grid3x3Top = topY - 10.0f * s;

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            const int idx = r * 3 + c;
            const float sx = grid3x3StartX + c * slotSize;
            const float sy = grid3x3Top - (r + 1) * slotSize;
            const ItemSlot& item = (craftingSlots != nullptr) ? craftingSlots[idx] : ItemSlot();
            drawMinecraftSlot(this, sx, sy, slotSize, item, mousePos, s, tooltip);
        }
    }

    // 3. Arrow pointing to Result Slot
    const float arrowX = grid3x3StartX + 3 * slotSize + 8.0f * s;
    const float arrowY = grid3x3Top - 2.0f * slotSize;
    drawTexturedRect(arrowX, arrowY, 18.0f * s, 18.0f * s, TextureTile::CraftingArrow);

    // 4. Result Slot
    const float resX = arrowX + 26.0f * s;
    const float resY = grid3x3Top - 2.2f * slotSize;
    const float resSize = 24.0f * s;
    drawMinecraftSlot(this, resX, resY, resSize, craftingResult, mousePos, s, tooltip);

    // 5. Bottom: "Inventory" Label + 3x9 Main Grid + 1x9 Hotbar Grid
    const float invStartX = left + 8.0f * s;
    const float invStartY = bottom + 74.0f * s;
    drawText(invStartX, invStartY + 10.0f * s, "Inventory", titleScale, glm::vec4(0.25f, 0.25f, 0.25f, 1.0f));

    drawInventoryGrid(this, invStartX, invStartY, slotSize, gap,
                      inventory, invCount, hotbar, hotbarCount, selectedHotbarSlot, mousePos, s, tooltip);

    // Tooltip and Cursor item
    renderTooltipAndCursor(this, tooltip, heldItem, mousePos, s, m_fbWidth);

    endUI();
}

void Renderer::drawCreativeInventory(int selectedHotbarSlot, const ItemSlot* hotbar, int hotbarCount,
                                     const ItemSlot& heldItem, const glm::vec2& mousePos,
                                     int activeTab, const std::string& searchQuery, int scrollRow) {
    const float s = m_uiScale;
    const float containerW = 195.0f * s;
    const float containerH = 136.0f * s;
    const float cx = static_cast<float>(m_fbWidth) * 0.5f;
    const float cy = static_cast<float>(m_fbHeight) * 0.5f;
    const float left = cx - containerW * 0.5f;
    const float bottom = cy - containerH * 0.5f;

    const float slotSize = 18.0f * s;
    const float gap = 0.0f;

    beginUI();

    // Dark backdrop
    drawRect(0.0f, 0.0f, static_cast<float>(m_fbWidth), static_cast<float>(m_fbHeight), glm::vec4(0.0f, 0.0f, 0.0f, 0.55f));

    // 1. Top Category Tabs
    const char* tabNames[5] = { "Building", "Tools", "Food", "Search", "All" };
    const TextureTile tabIcons[5] = {
        TextureTile::GrassTop, TextureTile::DiamondPickaxe,
        TextureTile::CookedPorkchop, TextureTile::SearchIcon, TextureTile::Apple
    };
    const float tabW = 26.0f * s;
    const float tabH = 20.0f * s;

    for (int t = 0; t < 5; ++t) {
        const float tx = left + 8.0f * s + t * (tabW + 2.0f * s);
        const float ty = bottom + containerH;
        const bool active = (t == activeTab);
        const bool hovered = (mousePos.x >= tx && mousePos.x <= tx + tabW &&
                              mousePos.y >= ty && mousePos.y <= ty + tabH);

        // Tab shape
        drawRect(tx - 1.0f * s, ty, tabW + 2.0f * s, tabH + 1.0f * s, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        drawRect(tx, ty, tabW, tabH, active ? glm::vec4(0.776f, 0.776f, 0.776f, 1.0f) :
                 (hovered ? glm::vec4(0.68f, 0.68f, 0.68f, 1.0f) : glm::vec4(0.55f, 0.55f, 0.55f, 1.0f)));
        drawTexturedRect(tx + (tabW - 14.0f * s) * 0.5f, ty + (tabH - 14.0f * s) * 0.5f, 14.0f * s, 14.0f * s, tabIcons[t]);
    }

    // 2. Main Minecraft Container Window
    drawMinecraftContainer(this, left, bottom, containerW, containerH, s);

    std::string tooltip;

    // 3. Search Bar / Header Title
    const float searchX = left + 80.0f * s;
    const float searchY = bottom + containerH - 16.0f * s;
    const float searchW = 90.0f * s;
    const float searchH = 12.0f * s;

    const float titleScale = std::max(1.0f, s * 0.62f);
    drawText(left + 8.0f * s, bottom + containerH - 6.0f * s, tabNames[activeTab], titleScale, glm::vec4(0.25f, 0.25f, 0.25f, 1.0f));

    // Sunken search input field
    drawRect(searchX, searchY, searchW, searchH, glm::vec4(0.22f, 0.22f, 0.22f, 1.0f));
    drawRect(searchX + 1.0f * s, searchY + 1.0f * s, searchW - 2.0f * s, searchH - 2.0f * s, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    drawTexturedRect(searchX + 2.0f * s, searchY + 1.0f * s, 10.0f * s, 10.0f * s, TextureTile::SearchIcon);

    const std::string displaySearch = searchQuery.empty() ? "Search..." : searchQuery;
    drawText(searchX + 14.0f * s, searchY + 8.5f * s, displaySearch, std::max(1.0f, s * 0.55f),
             searchQuery.empty() ? glm::vec4(0.5f, 0.5f, 0.5f, 1.0f) : glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // 4. 9x5 Catalog Item Grid (45 slots per view)
    const std::vector<BlockId> catalog = filterCreativeCatalog(activeTab, searchQuery);
    const float gridStartX = left + 8.0f * s;
    const float gridStartY = searchY - 4.0f * s;

    const int totalItems = static_cast<int>(catalog.size());
    const int startIdx = scrollRow * 9;

    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 9; ++col) {
            const int idx = startIdx + row * 9 + col;
            const float sx = gridStartX + col * (slotSize + gap);
            const float sy = gridStartY - (row + 1) * slotSize - row * gap;
            ItemSlot item;
            if (idx < totalItems) {
                const BlockId bId = catalog[idx];
                item = ItemSlot(bId, 1, isTool(bId) ? maxToolDurability(bId) : 0);
            }
            drawMinecraftSlot(this, sx, sy, slotSize, item, mousePos, s, tooltip);
        }
    }

    // 5. Scrollbar Track on Right
    const float scrollTrackX = gridStartX + 9 * slotSize + 2.0f * s;
    const float scrollTrackY = gridStartY - 5 * slotSize;
    const float scrollTrackW = 12.0f * s;
    const float scrollTrackH = 5 * slotSize;

    drawRect(scrollTrackX, scrollTrackY, scrollTrackW, scrollTrackH, glm::vec4(0.22f, 0.22f, 0.22f, 1.0f));
    drawRect(scrollTrackX + 1.0f * s, scrollTrackY + 1.0f * s, scrollTrackW - 2.0f * s, scrollTrackH - 2.0f * s, glm::vec4(0.55f, 0.55f, 0.55f, 1.0f));

    // Scrollbar Thumb Knob
    const float thumbH = 15.0f * s;
    const int maxScrollRows = std::max(0, (totalItems + 8) / 9 - 5);
    const float scrollPct = maxScrollRows > 0 ? (static_cast<float>(scrollRow) / maxScrollRows) : 0.0f;
    const float thumbY = scrollTrackY + (scrollTrackH - thumbH) * (1.0f - scrollPct);
    drawRect(scrollTrackX + 1.0f * s, thumbY, scrollTrackW - 2.0f * s, thumbH, glm::vec4(0.776f, 0.776f, 0.776f, 1.0f));
    drawRect(scrollTrackX + 2.0f * s, thumbY + 1.0f * s, scrollTrackW - 4.0f * s, thumbH - 2.0f * s, glm::vec4(0.88f, 0.88f, 0.88f, 1.0f));

    // 6. Bottom 1x9 Hotbar Slots
    const float hotbarY = bottom + 8.0f * s;
    for (int col = 0; col < 9 && col < hotbarCount; ++col) {
        const float sx = gridStartX + col * (slotSize + gap);
        drawMinecraftSlot(this, sx, hotbarY, slotSize, hotbar[col], mousePos, s, tooltip,
                          TextureTile::Count, (col == selectedHotbarSlot));
    }

    // Trash Can / Clear item Slot
    const float trashX = gridStartX + 9 * slotSize + 2.0f * s;
    const ItemSlot trashItem;
    drawMinecraftSlot(this, trashX, hotbarY, slotSize, trashItem, mousePos, s, tooltip, TextureTile::Destroy0);
    if (mousePos.x >= trashX && mousePos.x <= trashX + slotSize && mousePos.y >= hotbarY && mousePos.y <= hotbarY + slotSize) {
        tooltip = "Destroy Item / Clear";
    }

    renderTooltipAndCursor(this, tooltip, heldItem, mousePos, s, m_fbWidth);

    endUI();
}

} // namespace vox

#pragma once
#include "render/Font.hpp"
#include "render/FrameStats.hpp"
#include "render/ParticleSystem.hpp"
#include "render/Shader.hpp"
#include "render/Texture.hpp"
#include "world/Block.hpp"
#include "world/CraftingRecipes.hpp"
#include "world/ItemSlot.hpp"

#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace vox {

class World;
class Camera;
class EntityManager;
class Player;

// Owns all GPU resources and exposes the drawing API used by the game and the
// menus. World drawing is 3D; everything else is a 2D overlay in pixels
// (origin bottom-left) drawn through small immediate-style primitives.
class Renderer {
public:
    bool init();
    void shutdown();

    void setViewport(int width, int height);
    int width() const { return m_fbWidth; }
    int height() const { return m_fbHeight; }

    // Global 2D UI scale factor (pixels per UI unit). Set once per frame from
    // the GUI Scale setting; menus and the HUD both read it.
    void setUIScale(float scale);
    float uiScale() const { return m_uiScale; }

    void beginFrame(const glm::vec3& clearColor);
    void drawSky(const Camera& camera, float timeOfDay,
                 const glm::vec3& skyColor, const glm::vec3& fogColor, float sunlight);
    void drawWorld(const World& world, const Camera& camera,
                   const glm::vec3& fogColor, float fogStart, float fogEnd, float sunlight = 1.0f);
    void drawEntities(const EntityManager& entityManager, const World& world, const Camera& camera,
                      const glm::vec3& fogColor, float fogStart, float fogEnd, float sunlight = 1.0f);
    void drawPlayer(const Player& player, const World& world, const Camera& camera,
                    const glm::vec3& fogColor, float fogStart, float fogEnd, float sunlight = 1.0f);
    void drawFirstPersonArm(const Player& player, const World& world, const Camera& camera, float sunlight = 1.0f);
    void drawSelection(const Camera& camera, const glm::ivec3& block, BlockId blockId = BlockId::Grass);
    void drawBlockBreak(const Camera& camera, const glm::ivec3& block, BlockId blockId, int stage,
                        const World& world, float sunlight = 1.0f);

    void updateParticles(float dt, const World& world, const glm::vec3& playerPos) {
        m_particles.update(dt, world, playerPos);
    }
    void spawnDigParticles(const glm::vec3& blockPos, const glm::ivec3& normal, BlockId blockId,
                           const World& world, float sunlight = 1.0f, int count = 4) {
        m_particles.spawnDigParticles(blockPos, normal, static_cast<uint8_t>(blockId), world, sunlight, count);
    }
    void spawnBlockBreakParticles(const glm::vec3& blockPos, BlockId blockId,
                                  const World& world, float sunlight = 1.0f, int count = 24) {
        m_particles.spawnBlockBreakParticles(blockPos, static_cast<uint8_t>(blockId), world, sunlight, count);
    }
    void spawnFallingLeaf(const glm::vec3& pos, const glm::vec3& color = glm::vec3(0.24f, 0.65f, 0.18f)) {
        m_particles.spawnFallingLeaf(pos, color);
    }
    void spawnHitParticles(const glm::vec3& pos, const glm::vec3& hitDir, bool isCrit = false, int count = 12) {
        m_particles.spawnHitParticles(pos, hitDir, isCrit, count);
    }
    void spawnBloodSplatter(const glm::vec3& pos, const glm::vec3& hitDir, int count = 16) {
        m_particles.spawnBloodSplatter(pos, hitDir, count);
    }
    void clearParticles() { m_particles.clear(); }
    size_t particleCount() const { return m_particles.particleCount(); }

    // HUD shown while playing (crosshair, 9-slot hotbar, health hearts, hunger drumsticks, oxygen bubbles, XP bar).
    void drawHud(int selectedSlot, const ItemSlot* hotbar, int slotCount,
                 float health, float maxHealth,
                 float hunger, float maxHunger,
                 float oxygen, float maxOxygen,
                 bool inWater, float hurtTimer, float animTime,
                 bool isCreative = false);

    // Screen edge damage flash & vignette
    void drawHurtVignette(float hurtTimer, float healthRatio, float animTime);

    // RPG Region & Dimension Title Banner ("LEVEL 0" / "The Yellow Hell")
    void drawTitleBanner(const std::string& title, const std::string& subtitle, float alpha, float animTime);

    // Death Screen ("YOU DIED")
    void drawDeathScreen(float animTime, const glm::vec2& mousePos,
                         bool& outHoverRespawn, bool& outHoverQuit);

    // F3 debug overlay
    void drawDebugOverlay(const std::vector<std::string>& lines);

    // Per-frame render counters
    const FrameStats& stats() const { return m_stats; }
    FrameStats& stats() { return m_stats; }

    // Modern Minecraft Survival Inventory (Attachment 1: 4 Armor + Avatar + Offhand + 2x2 Crafting + 27 Main + 9 Hotbar)
    void drawSurvivalInventory(int selectedHotbarSlot,
                               const ItemSlot* hotbar, int hotbarCount,
                               const ItemSlot* inventory, int invCount,
                               const ItemSlot* armor, int armorCount,
                               const ItemSlot& offhand,
                               const ItemSlot* craftingSlots,
                               const ItemSlot& craftingResult,
                               const ItemSlot& heldItem,
                               const glm::vec2& mousePos,
                               bool recipeBookOpen = false);

    // Modern Minecraft Crafting Table Workbench (Attachment 3: 3x3 Crafting Grid + Arrow + Result + 27 Main + 9 Hotbar)
    void drawCraftingTableWorkbench(int selectedHotbarSlot,
                                    const ItemSlot* hotbar, int hotbarCount,
                                    const ItemSlot* inventory, int invCount,
                                    const ItemSlot* craftingSlots,
                                    const ItemSlot& craftingResult,
                                    const ItemSlot& heldItem,
                                    const glm::vec2& mousePos,
                                    bool recipeBookOpen = false);

    // Modern Minecraft Creative Item Catalog (Attachment 2: Category Tabs, Search Bar, 9x5 Grid with Scrollbar, 9 Hotbar)
    void drawCreativeInventory(int selectedHotbarSlot, const ItemSlot* hotbar, int hotbarCount,
                               const ItemSlot& heldItem, const glm::vec2& mousePos,
                               int activeTab = 0, const std::string& searchQuery = "", int scrollRow = 0);

    void drawDurabilityBar(float x, float y, float w, float h, int durability, int maxDurability);
    void drawBlockIcon(float x, float y, float w, float h, BlockId id);
    void drawTexturedRect(float x, float y, float w, float h, TextureTile tile,
                          const glm::vec4& tint = glm::vec4(1.0f));


    void drawUnderwaterOverlay(float time);
    void drawBackroomsHorrorOverlay(float time);

    // --- 2D primitives (screen pixels, origin bottom-left) ------------------
    void beginUI();
    void endUI();

    void drawRect(float x, float y, float w, float h, const glm::vec4& color);
    void drawGradientRect(float x, float y, float w, float h,
                          const glm::vec4& bottom, const glm::vec4& top);
    void drawTriangle(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c,
                      const glm::vec4& color);
    void drawText(float x, float y, const std::string& text, float scale,
                  const glm::vec4& color, float rotationRadians = 0.0f);

    float textWidth(const std::string& text, float scale) const;
    float textHeight(float scale) const { return m_font.textHeight(scale); }

    static std::string resolveAsset(const std::string& relativePath);

private:
    struct UIVertex {
        glm::vec2 pos;
        glm::vec4 color;
    };
    struct TextVertex {
        glm::vec2 pos;
        glm::vec2 uv;
    };
    struct SpriteVertex {
        glm::vec2 pos;
        glm::vec2 uv;
    };
    struct SkyVertex {
        glm::vec3 pos;
        glm::vec4 color;
    };

    void uploadUI(const UIVertex* vertices, int count);
    void initSky();

    Shader m_chunkShader;
    Shader m_skyShader;
    Shader m_skyDomeShader;
    Shader m_lineShader;
    Shader m_uiShader;
    Shader m_textShader;
    Shader m_spriteShader;
    Shader m_underwaterShader;
    Texture m_atlas;
    Font m_font;
    ParticleSystem m_particles;

    uint32_t m_lineVao = 0;
    uint32_t m_lineVbo = 0;
    uint32_t m_uiVao = 0;
    uint32_t m_uiVbo = 0;
    uint32_t m_textVao = 0;
    uint32_t m_textVbo = 0;
    uint32_t m_spriteVao = 0;
    uint32_t m_spriteVbo = 0;
    uint32_t m_entityVao = 0;
    uint32_t m_entityVbo = 0;
    uint32_t m_underwaterVao = 0;
    uint32_t m_underwaterVbo = 0;
    uint32_t m_skyVao = 0;
    uint32_t m_skyVbo = 0;
    uint32_t m_starVao = 0;
    uint32_t m_starVbo = 0;
    size_t m_starCount = 0;
    uint32_t m_skyDomeVao = 0;
    uint32_t m_skyDomeVbo = 0;
    uint32_t m_skyDomeEbo = 0;
    int m_skyDomeIndexCount = 0;

    int m_fbWidth = 1;
    int m_fbHeight = 1;
    float m_uiScale = 3.0f;
    FrameStats m_stats;
};

} // namespace vox

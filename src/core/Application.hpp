#pragma once

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include "audio/AudioEngine.hpp"
#include "core/Config.hpp"
#include "entity/EntityManager.hpp"
#include "input/Input.hpp"
#include "player/Player.hpp"
#include "render/Camera.hpp"
#include "render/Renderer.hpp"
#include "ui/Menu.hpp"
#include "world/Raycast.hpp"
#include "world/World.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace vox {

enum class GameState {
    MainMenu,
    SelectWorld,
    NewWorld,
    DeleteWorld,
    ConfirmDeleteWorld,
    Playing,
    Paused,
    Options,
    Inventory,
    GameOver,
};

// Top-level game object: owns the window, world, player, renderer, menus and
// input, and drives the main loop and state transitions.
class Application {
public:
    Application() = default;
    ~Application();

    bool init();
    void run();
    void shutdown();

private:
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double x, double y);
    static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);
    static void charCallback(GLFWwindow* window, unsigned int codepoint);

    // Menus / states
    void loadSplashes();
    std::string getRandomSplash();
    void buildMenus();
    void buildSelectWorldMenu();
    void buildNewWorldMenu();
    void buildDeleteWorldMenu();
    void buildConfirmDeleteMenu();
    Menu& activeMenu();
    void handleMenuInput();
    void handleCharInput(unsigned int codepoint);
    void updateMenuCamera(float dt);
    void openSelectWorld();
    void openNewWorld();
    void openDeleteWorld();
    void openConfirmDeleteWorld(const std::string& worldName, const std::string& path);
    void startNewWorld(const std::string& name, uint32_t seed, WorldType type = WorldType::Default);
    void loadWorld(const std::string& path);
    void startGame();
    void pauseGame();
    void resumeGame();
    void quitToTitle();
    void openOptions(GameState from);
    void closeOptions();
    void openInventory(bool withCraftingTable = false);
    void closeInventory();

    void handlePlayInput();
    void handleInventoryInput();
    void handleCreativeInventoryInput();
    void handleGameOverInput();
    void respawnPlayer();
    void updateInteraction(float dt);
    void rebuildDirtyMeshes();
    bool playerOccupies(const glm::ivec3& block) const;
    bool addItem(BlockId id, int count = 1);
    void discoverItem(BlockId id);
    bool isRecipeUnlocked(const ConsoleRecipeDef& recipe) const;
    std::vector<ConsoleRecipeDef> getUnlockedRecipes(int categoryIdx) const;
    bool canCraftRecipe(int catIdx, int recIdx) const;
    bool craftSelectedRecipe();
    void updateCraftingResult();
    void takeCraftingResult();
    void clearCraftingGrid();
    void toggleCreativeMode();
    void populateCreativeCatalog();

    static uint32_t parseSeed(const std::string& input);

    // Dimensions and RPG Title Banners
    void showTitleBanner(const std::string& title, const std::string& subtitle, float duration = 4.0f);
    void switchDimension(DimensionId targetDim, const glm::vec3& targetPos);
    glm::vec3 findSafeOverworldReturn(const glm::vec3& nearPos);

    // Day / Night cycle & Celestial lighting
    int moonPhase() const { return m_dayCount % 8; }
    float moonLightFactor() const;
    float computeSunlight() const;
    glm::vec3 computeSkyColor() const;
    glm::vec3 computeFogColor() const;

    // Settings / platform
    void applySettings();
    float computeUiScale() const;
    void setFullscreen(bool enabled);
    void setCursorCaptured(bool captured);
    glm::vec2 mouseInFramebuffer() const;

    // renderScene() draws only the current state; render() wraps it so the F3
    // overlay is composited last, on top of every state (playing, inventory and
    // all menus) without each path having to remember it.
    void render();
    void renderScene();
    void renderLoadingScreen(const std::string& title, float progress, const std::string& statusMessage);
    void drawDebugOverlayIfEnabled();

    // Optional headless capture for automated verification (env driven).
    void maybeCapture();
    void writeBmp(const std::string& path, int width, int height,
                  const std::vector<uint8_t>& rgb) const;
    void applyCaptureEnvironment();

    GLFWwindow* m_window = nullptr;
    GameState m_state = GameState::MainMenu;
    GameState m_optionsReturn = GameState::MainMenu;

    Input m_input;
    World m_world;
    EntityManager m_entityManager;
    Player m_player;
    Renderer m_renderer;
    Camera m_menuCamera;

    Menu m_mainMenu;
    Menu m_selectWorldMenu;
    Menu m_newWorldMenu;
    Menu m_deleteWorldMenu;
    Menu m_confirmDeleteMenu;
    Menu m_pauseMenu;
    Menu m_optionsMenu;
    std::vector<std::string> m_splashes;

    std::string m_activeWorldName = "World 1";
    std::string m_activeWorldPath;
    uint32_t m_activeWorldSeed = 1337;
    WorldType m_activeWorldType = WorldType::Default;

    std::string m_newWorldName = "World 1";
    std::string m_newWorldSeedInput;
    uint32_t m_newWorldSeed = 1337;
    WorldType m_newWorldType = WorldType::Default;

    std::string m_pendingDeleteWorldName;
    std::string m_pendingDeleteWorldPath;

    ItemSlot m_hotbar[9] = {};
    ItemSlot m_inventory[27] = {};
    ItemSlot m_armor[4] = {};
    ItemSlot m_offhand;
    ItemSlot m_craftingSlots[9] = {};
    ItemSlot m_craftingResult;
    ItemSlot m_heldItem;
    int m_selectedSlot = 0;

    bool m_discoveredItems[static_cast<size_t>(BlockId::Count)] = {};
    int m_craftingCategory = 0;
    int m_selectedRecipe = 0;
    bool m_isCraftingTableOpen = false;
    bool m_recipeBookOpen = false;

    bool m_creativeMode = false;
    int m_creativeTab = 0;
    std::string m_creativeSearchQuery;
    int m_creativeScrollRow = 0;
    float m_timeOfDay = 0.22f; // Starts in the morning (~08:30 AM)
    int m_dayCount = 0;

    // Dimension travel & RPG title banner state
    std::string m_bannerTitle;
    std::string m_bannerSubtitle;
    float m_bannerTimer = 0.0f;
    float m_bannerDuration = 4.0f;
    float m_dimensionCooldown = 0.0f;
    glm::vec3 m_overworldReturnPos{0.5f, 65.0f, 0.5f};

    // Mining / Block breaking state
    bool m_isMining = false;
    glm::ivec3 m_miningBlock{0};
    float m_miningProgress = 0.0f;
    float m_digSoundTimer = 0.0f;

    // Eating state
    float m_eatingTimer = 0.0f;
    float m_eatSoundTimer = 0.0f;

    // Simulation ticks
    float m_fluidTickTimer = 0.0f;
    float m_ecologyTickTimer = 0.0f;

    RayHit m_target;
    double m_lastTime = 0.0;
    double m_uiTime = 0.0;
    bool m_wireframe = false;
    bool m_showDebugOverlay = false; // F3

    // Runtime settings (exposed through the Options menu).
    float m_mouseSensitivity = 0.12f;
    float m_fov = 70.0f;
    int m_viewDistanceChunks = 8;
    int m_guiScale = 0; // 0 = auto, otherwise a fixed multiplier (1..4)
    bool m_vsync = true;
    bool m_fullscreen = false;
    bool m_viewBobbing = true;
    float m_masterVolume = 0.8f;
    float m_sfxVolume = 0.8f;
    float m_ambientVolume = 0.35f;

    AudioEngine m_audioEngine;

    int m_fbWidth = config::WINDOW_WIDTH;
    int m_fbHeight = config::WINDOW_HEIGHT;

    std::string m_capturePath;
    int m_captureFrame = 90;
    int m_capturePauseAt = -1;
    int m_captureOptionsAt = -1;
    int m_frameCounter = 0;
    bool m_captureDone = false;
};

} // namespace vox

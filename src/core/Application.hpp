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
    void openInventory();
    void closeInventory();

    // Gameplay
    void handlePlayInput();
    void handleInventoryInput();
    void handleGameOverInput();
    void respawnPlayer();
    void updateInteraction(float dt);
    void rebuildDirtyMeshes();
    bool playerOccupies(const glm::ivec3& block) const;
    bool addItem(BlockId id, int count = 1);
    void updateCrafting();
    void takeCraftResult();
    void toggleCreativeMode();
    void populateCreativeCatalog();

    // Day / Night cycle & Celestial lighting
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
    uint32_t m_newWorldSeed = 1337;
    WorldType m_newWorldType = WorldType::Default;

    std::string m_pendingDeleteWorldName;
    std::string m_pendingDeleteWorldPath;

    ItemSlot m_hotbar[8] = {};
    ItemSlot m_inventory[24] = {};
    ItemSlot m_craftGrid[4] = {};
    ItemSlot m_craftResult;
    ItemSlot m_heldItem;
    int m_selectedSlot = 0;

    bool m_creativeMode = false;
    float m_timeOfDay = 0.22f; // Starts in the morning (~08:30 AM)

    // Mining / Block breaking state
    bool m_isMining = false;
    glm::ivec3 m_miningBlock{0};
    float m_miningProgress = 0.0f;
    float m_digSoundTimer = 0.0f;

    // Eating state
    float m_eatingTimer = 0.0f;
    float m_eatSoundTimer = 0.0f;


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

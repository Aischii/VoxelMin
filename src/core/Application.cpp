#include "core/Application.hpp"
#include "core/Log.hpp"
#include "world/Chunk.hpp"
#include "world/ChunkMesher.hpp"
#include "world/WorldSave.hpp"

#include <GL/glew.h>
#include <stb/stb_image.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <random>
#include <string>
#include <vector>

namespace vox {

Application::~Application() {
    if (m_window) shutdown();
}

bool Application::init() {
    if (!glfwInit()) {
        log::error("glfwInit failed");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(config::WINDOW_WIDTH, config::WINDOW_HEIGHT,
                                config::WINDOW_TITLE, nullptr, nullptr);
    if (!m_window) {
        log::error("Failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    // Set application window icon
    {
        int iconW = 0, iconH = 0, iconChannels = 0;
        const std::string iconPath = Renderer::resolveAsset("assets/icon.png");
        stbi_uc* iconData = stbi_load(iconPath.c_str(), &iconW, &iconH, &iconChannels, 4);
        if (iconData) {
            GLFWimage iconImage;
            iconImage.width = iconW;
            iconImage.height = iconH;
            iconImage.pixels = iconData;
            glfwSetWindowIcon(m_window, 1, &iconImage);
            stbi_image_free(iconData);
            log::info("Window icon set (%dx%d)", iconW, iconH);
        }
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(m_vsync ? 1 : 0);

    const char* glVer = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* glRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    log::info("OpenGL Context created: %s (%s)", glVer ? glVer : "null", glRenderer ? glRenderer : "null");

    glewExperimental = GL_TRUE;
    const GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK && glewStatus != GLEW_ERROR_NO_GLX_DISPLAY) {
        log::error("glewInit failed: %s (code %u)", reinterpret_cast<const char*>(glewGetErrorString(glewStatus)), (unsigned)glewStatus);
        return false;
    }
    glGetError(); // clear GLEW's benign GL_INVALID_ENUM

    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, &Application::keyCallback);
    glfwSetMouseButtonCallback(m_window, &Application::mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, &Application::cursorPosCallback);
    glfwSetScrollCallback(m_window, &Application::scrollCallback);
    glfwSetCharCallback(m_window, &Application::charCallback);

    if (!m_renderer.init()) {
        log::error("Renderer initialisation failed");
        return false;
    }

    log::info("VoxelMin v%s starting...", config::VERSION);
    log::info("Generating lightweight panorama world for main menu...");
    m_activeWorldName = "World 1";
    m_activeWorldPath = WorldSave::getWorldPath(m_activeWorldName);
    m_activeWorldSeed = config::WORLD_SEED;
    m_world.init(8, 8, m_activeWorldSeed);
    m_world.generate(WorldType::Default);
    m_player.spawnAt(m_world,
                     static_cast<float>(m_world.widthBlocks()) * 0.5f,
                     static_cast<float>(m_world.depthBlocks()) * 0.5f);
    m_entityManager.spawnDefaults(m_world, m_activeWorldSeed);

    // Pre-mesh menu chunks immediately for instantaneous rendering
    for (const auto& chunk : m_world.chunks()) {
        if (!chunk) continue;
        std::vector<Vertex> opVertices, trVertices;
        std::vector<uint32_t> opIndices, trIndices;
        buildChunkGeometry(m_world, *chunk, opVertices, opIndices, trVertices, trIndices);
        chunk->mesh.upload(opVertices, opIndices);
        chunk->transparentMesh.upload(trVertices, trIndices);
        chunk->dirty = false;
    }

    log::info("Menu panorama ready: %d x %d x %d blocks",
              m_world.widthBlocks(), m_world.heightBlocks(), m_world.depthBlocks());

    loadSplashes();
    buildMenus();
    applySettings();
    updateMenuCamera(0.0f);

    m_state = GameState::MainMenu;
    setCursorCaptured(false);
    applyCaptureEnvironment();

    m_lastTime = glfwGetTime();
    return true;
}

void Application::run() {
    while (!glfwWindowShouldClose(m_window)) {
        const double now = glfwGetTime();
        float dt = static_cast<float>(now - m_lastTime);
        m_lastTime = now;
        if (dt > 0.1f) dt = 0.1f; // avoid huge steps after a pause
        m_uiTime += static_cast<double>(dt);

        m_input.newFrame();
        glfwPollEvents();

        int fbWidth = 0;
        int fbHeight = 0;
        glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
        if (fbWidth == 0 || fbHeight == 0) {
            glfwWaitEventsTimeout(0.1);
            continue;
        }
        m_fbWidth = fbWidth;
        m_fbHeight = fbHeight;
        m_renderer.setViewport(fbWidth, fbHeight);

        // F3 works in every state, so it is handled before the state dispatch
        // rather than inside handlePlayInput().
        if (m_input.keyPressed(GLFW_KEY_F3)) {
            m_showDebugOverlay = !m_showDebugOverlay;
        }

        if (m_state == GameState::Playing) {
            if (m_input.keyPressed(GLFW_KEY_ESCAPE)) {
                pauseGame();
            } else {
                handlePlayInput();
                m_player.update(dt, m_input, m_world);
                m_entityManager.update(dt, m_world, m_player.position());
                m_renderer.updateParticles(dt, m_world, m_player.position());
                updateInteraction();
            }
        } else if (m_state == GameState::Inventory) {
            handleInventoryInput();
            m_player.update(dt, m_input, m_world);
            m_entityManager.update(dt, m_world, m_player.position());
            m_renderer.updateParticles(dt, m_world, m_player.position());
        } else {
            updateMenuCamera(dt);
            handleMenuInput();
        }

        rebuildDirtyMeshes();
        render();
        maybeCapture();
        // Fold the finished frame into the rolling averages. Done after render
        // so the counters drawn by the F3 overlay are the ones just rendered.
        m_renderer.stats().accumulate(dt);
        glfwSwapBuffers(m_window);
    }
}

void Application::shutdown() {
    if (m_state == GameState::Playing || m_state == GameState::Paused || m_state == GameState::Inventory) {
        const std::string savePath = m_activeWorldPath.empty() ? WorldSave::getWorldPath(m_activeWorldName) : m_activeWorldPath;
        WorldSave::saveGame(savePath, m_activeWorldName, m_activeWorldSeed,
                            m_world, m_player, m_selectedSlot, m_hotbar, m_inventory);
    }
    m_renderer.shutdown();
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

// ---------------------------------------------------------------------------
// Menus & Splash Text
// ---------------------------------------------------------------------------

void Application::loadSplashes() {
    m_splashes.clear();
    const std::string path = Renderer::resolveAsset("assets/splashes.txt");
    std::ifstream file(path);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
                line.pop_back();
            }
            if (!line.empty()) {
                m_splashes.push_back(line);
            }
        }
    }

    if (m_splashes.empty()) {
        m_splashes = {
            "Smooth Lighting & Shadows!",
            "16x16 Pixel Perfection!",
            "Legacy Console aesthetic!",
            "Creative Inventory unlocked!",
            "Zero Compiler Warnings!",
            "Contact AO activated!",
            "Now with Sunlight propagation!",
        };
    }
    log::info("Loaded %zu splash text entries from '%s'", m_splashes.size(), path.c_str());
}

std::string Application::getRandomSplash() {
    if (m_splashes.empty()) return "VoxelMin Alpha!";
    static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<size_t> dist(0, m_splashes.size() - 1);
    return m_splashes[dist(rng)];
}

void Application::buildMenus() {
    m_mainMenu.clear();
    m_mainMenu.logo(true)
              .title("VOXELMIN")
              .subtitle(std::string("Singleplayer Alpha v") + config::VERSION)
              .splash(getRandomSplash())
              .footer("Up/Down  Select     Enter  Confirm");
    m_mainMenu.addButton("Play Game", [this] { openSelectWorld(); });
    m_mainMenu.addButton("Options", [this] { openOptions(GameState::MainMenu); });
    m_mainMenu.addButton("Quit Game", [this] {
        const std::string savePath = m_activeWorldPath.empty() ? WorldSave::getWorldPath(m_activeWorldName) : m_activeWorldPath;
        WorldSave::saveGame(savePath, m_activeWorldName, m_activeWorldSeed,
                            m_world, m_player, m_selectedSlot, m_hotbar, m_inventory);
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    });
    m_mainMenu.resetSelection();

    buildSelectWorldMenu();
    buildNewWorldMenu();
    buildDeleteWorldMenu();
    buildConfirmDeleteMenu();

    m_pauseMenu.clear();
    m_pauseMenu.logo(false)
               .title("Game Menu")
               .subtitle("World: " + m_activeWorldName)
               .footer("Up/Down  Select     Enter  Confirm     Esc  Back to Game");
    m_pauseMenu.addButton("Back to Game", [this] { resumeGame(); });
    m_pauseMenu.addButton("Options", [this] { openOptions(GameState::Paused); });
    m_pauseMenu.addButton("Save and Quit to Title", [this] {
        const std::string savePath = m_activeWorldPath.empty() ? WorldSave::getWorldPath(m_activeWorldName) : m_activeWorldPath;
        WorldSave::saveGame(savePath, m_activeWorldName, m_activeWorldSeed,
                            m_world, m_player, m_selectedSlot, m_hotbar, m_inventory);
        quitToTitle();
    });
    m_pauseMenu.resetSelection();

    m_optionsMenu.clear();
    m_optionsMenu.logo(false)
                 .title("Options")
                 .footer("Up/Down  Select     Left/Right  Change     Esc  Back");
    m_optionsMenu.addOption(
        "Mouse Sensitivity",
        [this] { return std::to_string(static_cast<int>(std::lround(m_mouseSensitivity / 0.12f * 100.0f))) + "%"; },
        [this](int d) {
            m_mouseSensitivity = std::clamp(m_mouseSensitivity + 0.02f * static_cast<float>(d), 0.02f, 0.40f);
            applySettings();
        });
    m_optionsMenu.addOption(
        "Field of View",
        [this] { return std::to_string(static_cast<int>(std::lround(m_fov))); },
        [this](int d) {
            m_fov = std::clamp(m_fov + 5.0f * static_cast<float>(d), 60.0f, 110.0f);
            applySettings();
        });
    m_optionsMenu.addOption(
        "View Distance",
        [this] { return std::to_string(m_viewDistanceChunks) + " chunks"; },
        [this](int d) {
            m_viewDistanceChunks = std::clamp(m_viewDistanceChunks + d, 4, 12);
            applySettings();
        });
    m_optionsMenu.addOption(
        "GUI Scale",
        [this] { return m_guiScale == 0 ? std::string("AUTO") : std::to_string(m_guiScale); },
        [this](int d) {
            m_guiScale += d;
            if (m_guiScale < 0) m_guiScale = 4;
            if (m_guiScale > 4) m_guiScale = 0;
        });
    m_optionsMenu.addOption(
        "VSync",
        [this] { return std::string(m_vsync ? "ON" : "OFF"); },
        [this](int) {
            m_vsync = !m_vsync;
            applySettings();
        });
    m_optionsMenu.addOption(
        "Fullscreen",
        [this] { return std::string(m_fullscreen ? "ON" : "OFF"); },
        [this](int) {
            setFullscreen(!m_fullscreen);
            applySettings();
        });
    m_optionsMenu.addOption(
        "Wireframe",
        [this] { return std::string(m_wireframe ? "ON" : "OFF"); },
        [this](int) { m_wireframe = !m_wireframe; });
    m_optionsMenu.addButton("Back", [this] { closeOptions(); });
    m_optionsMenu.resetSelection();
}

void Application::buildSelectWorldMenu() {
    m_selectWorldMenu.clear();
    m_selectWorldMenu.logo(false)
                     .title("Select World")
                     .subtitle("Choose a world to play or create a new one")
                     .footer("Up/Down  Select     Enter  Confirm     Del  Delete     Esc  Back");

    const auto savedWorlds = WorldSave::listSavedWorlds();
    for (const auto& w : savedWorlds) {
        const std::string label = w.name + "  [Seed: " + std::to_string(w.seed) + "]";
        m_selectWorldMenu.addButton(label, [this, path = w.path] {
            loadWorld(path);
        });
    }

    m_selectWorldMenu.addButton("Create New World", [this] { openNewWorld(); });
    if (!savedWorlds.empty()) {
        m_selectWorldMenu.addButton("Delete a World", [this] { openDeleteWorld(); });
    }
    m_selectWorldMenu.addButton("Back", [this] { m_state = GameState::MainMenu; });
    m_selectWorldMenu.resetSelection();
}

void Application::buildDeleteWorldMenu() {
    m_deleteWorldMenu.clear();
    m_deleteWorldMenu.logo(false)
                     .title("Delete World")
                     .subtitle("Select a world to permanently delete")
                     .footer("Up/Down  Select     Enter  Confirm     Esc  Back");

    const auto savedWorlds = WorldSave::listSavedWorlds();
    for (const auto& w : savedWorlds) {
        const std::string label = "Delete: " + w.name + "  [Seed: " + std::to_string(w.seed) + "]";
        m_deleteWorldMenu.addButton(label, [this, name = w.name, path = w.path] {
            openConfirmDeleteWorld(name, path);
        });
    }

    m_deleteWorldMenu.addButton("Back", [this] { openSelectWorld(); });
    m_deleteWorldMenu.resetSelection();
}

void Application::buildConfirmDeleteMenu() {
    m_confirmDeleteMenu.clear();
    m_confirmDeleteMenu.logo(false)
                       .title("Delete World?")
                       .subtitle("Are you sure you want to delete '" + m_pendingDeleteWorldName + "'?")
                       .footer("Up/Down  Select     Enter  Confirm     Esc  Cancel");

    m_confirmDeleteMenu.addButton("Yes, Delete World", [this] {
        WorldSave::deleteWorld(m_pendingDeleteWorldPath);
        openSelectWorld();
    });
    m_confirmDeleteMenu.addButton("Cancel", [this] {
        openDeleteWorld();
    });
    m_confirmDeleteMenu.resetSelection();
}

void Application::openDeleteWorld() {
    buildDeleteWorldMenu();
    m_state = GameState::DeleteWorld;
    setCursorCaptured(false);
}

void Application::openConfirmDeleteWorld(const std::string& worldName, const std::string& path) {
    m_pendingDeleteWorldName = worldName;
    m_pendingDeleteWorldPath = path;
    buildConfirmDeleteMenu();
    m_state = GameState::ConfirmDeleteWorld;
    setCursorCaptured(false);
}

void Application::buildNewWorldMenu() {
    m_newWorldMenu.clear();
    m_newWorldMenu.logo(false)
                  .title("Create New World")
                  .subtitle("Configure world name, world type, and seed")
                  .footer("Type name     Left/Right  Change     Enter  Select     Esc  Back");

    m_newWorldMenu.addOption(
        "World Name",
        [this] {
            const bool isSelected = (m_state == GameState::NewWorld && m_newWorldMenu.selection() == 0);
            const bool blink = (static_cast<int>(m_uiTime * 3.0) % 2 == 0);
            return m_newWorldName + (isSelected ? (blink ? "_" : " ") : "");
        },
        [](int) {});

    m_newWorldMenu.addOption(
        "World Type",
        [this] { return std::string(worldTypeName(m_newWorldType)); },
        [this](int d) {
            const int count = static_cast<int>(WorldType::Count);
            int cur = static_cast<int>(m_newWorldType);
            cur = (cur + d + count) % count;
            m_newWorldType = static_cast<WorldType>(cur);
        });

    m_newWorldMenu.addOption(
        "World Seed",
        [this] { return std::to_string(m_newWorldSeed); },
        [this](int d) {
            int seedInt = static_cast<int>(m_newWorldSeed) + d * 100;
            if (seedInt < 1) seedInt = 1;
            m_newWorldSeed = static_cast<uint32_t>(seedInt);
        });

    m_newWorldMenu.addButton("Roll Random Seed", [this] {
        m_newWorldSeed = static_cast<uint32_t>(std::rand() % 899999 + 1000);
    });

    m_newWorldMenu.addButton("Create World", [this] {
        startNewWorld(m_newWorldName, m_newWorldSeed, m_newWorldType);
    });

    m_newWorldMenu.addButton("Cancel", [this] { openSelectWorld(); });
    m_newWorldMenu.resetSelection();
}

Menu& Application::activeMenu() {
    switch (m_state) {
        case GameState::SelectWorld: return m_selectWorldMenu;
        case GameState::NewWorld: return m_newWorldMenu;
        case GameState::DeleteWorld: return m_deleteWorldMenu;
        case GameState::ConfirmDeleteWorld: return m_confirmDeleteMenu;
        case GameState::Paused: return m_pauseMenu;
        case GameState::Options: return m_optionsMenu;
        case GameState::MainMenu:
        default: return m_mainMenu;
    }
}

void Application::handleCharInput(unsigned int codepoint) {
    if (m_state == GameState::NewWorld && m_newWorldMenu.selection() == 0) {
        if (codepoint >= 32 && codepoint <= 126 && m_newWorldName.size() < 24) {
            m_newWorldName.push_back(static_cast<char>(codepoint));
        }
    }
}

void Application::handleMenuInput() {
    if (m_state == GameState::Options) {
        if (m_input.keyPressed(GLFW_KEY_ESCAPE)) {
            closeOptions();
            return;
        }
    } else if (m_state == GameState::SelectWorld) {
        if (m_input.keyPressed(GLFW_KEY_ESCAPE)) {
            m_state = GameState::MainMenu;
            return;
        }
        if (m_input.keyPressed(GLFW_KEY_DELETE)) {
            const auto savedWorlds = WorldSave::listSavedWorlds();
            const int sel = m_selectWorldMenu.selection();
            if (sel >= 0 && static_cast<size_t>(sel) < savedWorlds.size()) {
                openConfirmDeleteWorld(savedWorlds[sel].name, savedWorlds[sel].path);
                return;
            }
        }
    } else if (m_state == GameState::DeleteWorld) {
        if (m_input.keyPressed(GLFW_KEY_ESCAPE)) {
            openSelectWorld();
            return;
        }
    } else if (m_state == GameState::ConfirmDeleteWorld) {
        if (m_input.keyPressed(GLFW_KEY_ESCAPE)) {
            openDeleteWorld();
            return;
        }
    } else if (m_state == GameState::NewWorld) {
        if (m_input.keyPressed(GLFW_KEY_ESCAPE)) {
            openSelectWorld();
            return;
        }
        if (m_newWorldMenu.selection() == 0) {
            if (m_input.keyPressed(GLFW_KEY_BACKSPACE)) {
                if (!m_newWorldName.empty()) {
                    m_newWorldName.pop_back();
                }
            }
        }
    } else if (m_state == GameState::Paused) {
        if (m_input.keyPressed(GLFW_KEY_ESCAPE)) {
            resumeGame();
            return;
        }
    }

    Menu& menu = activeMenu();

    if (m_input.keyPressed(GLFW_KEY_UP) || (m_input.keyPressed(GLFW_KEY_W) && !(m_state == GameState::NewWorld && m_newWorldMenu.selection() == 0))) {
        menu.moveSelection(-1);
    }
    if (m_input.keyPressed(GLFW_KEY_DOWN) || (m_input.keyPressed(GLFW_KEY_S) && !(m_state == GameState::NewWorld && m_newWorldMenu.selection() == 0))) {
        menu.moveSelection(1);
    }
    if (m_input.keyPressed(GLFW_KEY_LEFT) || (m_input.keyPressed(GLFW_KEY_A) && !(m_state == GameState::NewWorld && m_newWorldMenu.selection() == 0))) {
        menu.adjustSelected(-1);
    }
    if (m_input.keyPressed(GLFW_KEY_RIGHT) || (m_input.keyPressed(GLFW_KEY_D) && !(m_state == GameState::NewWorld && m_newWorldMenu.selection() == 0))) {
        menu.adjustSelected(1);
    }

    const bool confirm = m_input.keyPressed(GLFW_KEY_ENTER) ||
                         m_input.keyPressed(GLFW_KEY_KP_ENTER);
    if (confirm) {
        menu.activate();
        return;
    }

    const glm::vec2 mouse = mouseInFramebuffer();
    const bool mouseMoved = m_input.mouseDeltaX() != 0.0f || m_input.mouseDeltaY() != 0.0f;
    if (mouseMoved) {
        const int hovered = menu.rowAt(mouse.x, mouse.y);
        if (hovered >= 0) menu.setSelection(hovered);
    }

    const int clickedLeft = m_input.mousePressed(GLFW_MOUSE_BUTTON_LEFT) ? menu.rowAt(mouse.x, mouse.y) : -1;
    const int clickedRight = m_input.mousePressed(GLFW_MOUSE_BUTTON_RIGHT) ? menu.rowAt(mouse.x, mouse.y) : -1;

    if (clickedLeft >= 0) {
        menu.setSelection(clickedLeft);
        if (const Menu::Row* row = menu.getRow(clickedLeft)) {
            if (row->isOption()) {
                const float midX = row->x + row->w * 0.5f;
                menu.adjustSelected(mouse.x < midX ? -1 : 1);
            } else {
                menu.activate();
            }
        }
    } else if (clickedRight >= 0) {
        menu.setSelection(clickedRight);
        if (const Menu::Row* row = menu.getRow(clickedRight)) {
            if (row->isOption()) {
                menu.adjustSelected(-1);
            }
        }
    }
}

void Application::updateMenuCamera(float dt) {
    (void)dt;
    const float t = static_cast<float>(m_uiTime) * 0.06f;
    const glm::vec3 center(m_world.widthBlocks() * 0.5f, 30.0f, m_world.depthBlocks() * 0.5f);
    const float radius = 62.0f;
    const glm::vec3 position = center + glm::vec3(std::cos(t) * radius, 20.0f, std::sin(t) * radius);
    m_menuCamera.setPosition(position);

    const glm::vec3 direction = glm::normalize(center - position);
    m_menuCamera.setRotation(glm::degrees(std::atan2(direction.z, direction.x)),
                             glm::degrees(std::asin(direction.y)));
}

void Application::openSelectWorld() {
    buildSelectWorldMenu();
    m_state = GameState::SelectWorld;
    setCursorCaptured(false);
}

void Application::openNewWorld() {
    buildNewWorldMenu();
    m_state = GameState::NewWorld;
    setCursorCaptured(false);
}

void Application::startNewWorld(const std::string& name, uint32_t seed, WorldType type) {
    m_activeWorldName = name.empty() ? "World 1" : name;
    m_activeWorldPath = WorldSave::getWorldPath(m_activeWorldName);
    m_activeWorldSeed = seed;
    m_activeWorldType = type;
    log::info("Starting new world '%s' (type %s, seed %u)...", m_activeWorldName.c_str(), worldTypeName(type), seed);

    auto onProgress = [this](float progress, const std::string& status) {
        renderLoadingScreen(m_activeWorldName, progress, status);
    };

    onProgress(0.02f, "Initializing terrain matrix...");
    m_world.init(config::WORLD_CHUNKS_X, config::WORLD_CHUNKS_Z, seed);
    m_world.generate(type, onProgress);

    for (const auto& v : m_world.villages()) {
        log::info("  Village '%s' (type %d) at (%.1f, %.1f, %.1f)", v.name.c_str(), v.templateType, v.center.x, v.center.y, v.center.z);
    }

    onProgress(0.85f, "Placing player at spawn point...");
    m_player.spawnAt(m_world,
                     static_cast<float>(m_world.widthBlocks()) * 0.5f,
                     static_cast<float>(m_world.depthBlocks()) * 0.5f);
    m_player.setFlying(false);
    m_selectedSlot = 0;

    onProgress(0.88f, "Populating fauna & resident pigmen...");
    m_entityManager.spawnDefaults(m_world, seed);
    m_renderer.clearParticles();

    // Pre-mesh all chunks in the world with smooth progress bar updates!
    const auto& chunks = m_world.chunks();
    const size_t totalChunks = chunks.size();
    for (size_t i = 0; i < totalChunks; ++i) {
        Chunk* chunk = chunks[i].get();
        if (!chunk) continue;
        std::vector<Vertex> opVertices, trVertices;
        std::vector<uint32_t> opIndices, trIndices;
        buildChunkGeometry(m_world, *chunk, opVertices, opIndices, trVertices, trIndices);
        chunk->mesh.upload(opVertices, opIndices);
        chunk->transparentMesh.upload(trVertices, trIndices);
        chunk->dirty = false;

        if (i % 64 == 0 || i == totalChunks - 1) {
            float p = 0.90f + 0.08f * (static_cast<float>(i + 1) / static_cast<float>(totalChunks));
            onProgress(p, "Building terrain chunk meshes (" + std::to_string(i + 1) + "/" + std::to_string(totalChunks) + ")...");
        }
    }

    onProgress(0.99f, "Saving initial world snapshot...");
    WorldSave::saveGame(m_activeWorldPath, m_activeWorldName, m_activeWorldSeed,
                        m_world, m_player, m_selectedSlot, m_hotbar, m_inventory);

    onProgress(1.00f, "Entering world...");
    m_state = GameState::Playing;
    setCursorCaptured(true);
}

void Application::loadWorld(const std::string& path) {
    m_activeWorldPath = path;
    auto onProgress = [this](float progress, const std::string& status) {
        renderLoadingScreen(m_activeWorldName, progress, status);
    };

    onProgress(0.05f, "Reading saved world data...");
    if (WorldSave::loadGame(path, m_activeWorldName, m_activeWorldSeed, m_world, m_player, m_selectedSlot, m_hotbar, m_inventory)) {
        m_world.setSeed(m_activeWorldSeed);
        onProgress(0.40f, "Populating fauna & entities...");
        m_entityManager.spawnDefaults(m_world, m_activeWorldSeed);
        m_renderer.clearParticles();

        const auto& chunks = m_world.chunks();
        const size_t totalChunks = chunks.size();
        for (size_t i = 0; i < totalChunks; ++i) {
            Chunk* chunk = chunks[i].get();
            if (!chunk) continue;
            std::vector<Vertex> opVertices, trVertices;
            std::vector<uint32_t> opIndices, trIndices;
            buildChunkGeometry(m_world, *chunk, opVertices, opIndices, trVertices, trIndices);
            chunk->mesh.upload(opVertices, opIndices);
            chunk->transparentMesh.upload(trVertices, trIndices);
            chunk->dirty = false;

            if (i % 64 == 0 || i == totalChunks - 1) {
                float p = 0.50f + 0.48f * (static_cast<float>(i + 1) / static_cast<float>(totalChunks));
                onProgress(p, "Building terrain chunk meshes (" + std::to_string(i + 1) + "/" + std::to_string(totalChunks) + ")...");
            }
        }

        onProgress(1.00f, "Entering world...");
        m_state = GameState::Playing;
        setCursorCaptured(true);
    }
}

void Application::startGame() {
    openSelectWorld();
}

void Application::pauseGame() {
    m_state = GameState::Paused;
    m_pauseMenu.subtitle("World: " + m_activeWorldName);
    m_pauseMenu.resetSelection();
    setCursorCaptured(false);
}

void Application::resumeGame() {
    m_state = GameState::Playing;
    setCursorCaptured(true);
}

void Application::quitToTitle() {
    m_state = GameState::MainMenu;
    m_mainMenu.splash(getRandomSplash());
    m_mainMenu.resetSelection();
    setCursorCaptured(false);
}

void Application::openOptions(GameState from) {
    m_optionsReturn = from;
    m_state = GameState::Options;
    m_optionsMenu.resetSelection();
}

void Application::closeOptions() {
    m_state = m_optionsReturn;
    if (m_state == GameState::Paused) m_pauseMenu.resetSelection();
    else m_mainMenu.resetSelection();
}
void Application::openInventory() {
    m_state = GameState::Inventory;
    setCursorCaptured(false);
}

void Application::closeInventory() {
    m_state = GameState::Playing;
    setCursorCaptured(true);
}

void Application::handleInventoryInput() {
    if (m_input.keyPressed(GLFW_KEY_E) || m_input.keyPressed(GLFW_KEY_ESCAPE)) {
        closeInventory();
        return;
    }

    const glm::vec2 mouse = mouseInFramebuffer();
    const float s = computeUiScale();
    const float slot = 18.0f * s;
    const float gap = 3.0f * s;
    const int cols = 8;
    const int mainRows = 3;

    const float gridW = cols * slot + (cols - 1) * gap;
    const float mainH = mainRows * slot + (mainRows - 1) * gap;
    const float hotbarH = slot;
    const float pad = 12.0f * s;
    const float headerH = 20.0f * s;
    const float sectionGap = 10.0f * s;

    const float containerW = gridW + 2.0f * pad;
    const float containerH = headerH + mainH + sectionGap + hotbarH + 2.0f * pad + 16.0f * s;

    const float cx = static_cast<float>(m_fbWidth) * 0.5f;
    const float cy = static_cast<float>(m_fbHeight) * 0.5f;
    const float left = cx - containerW * 0.5f;
    const float bottom = cy - containerH * 0.5f;

    const float gridLeft = left + pad;
    const float mainGridTop = bottom + containerH - pad - headerH;
    const float hotbarTop = mainGridTop - mainH - sectionGap * 0.5f;
    const float hotbarY = hotbarTop - slot - 4.0f * s;

    int hoveredInvIdx = -1;
    int hoveredHotbarIdx = -1;

    for (int row = 0; row < mainRows; ++row) {
        for (int col = 0; col < cols; ++col) {
            const int idx = row * cols + col;
            if (idx >= 24) break;
            const float x = gridLeft + col * (slot + gap);
            const float y = mainGridTop - (row + 1) * slot - row * gap;
            if (mouse.x >= x && mouse.x <= x + slot && mouse.y >= y && mouse.y <= y + slot) {
                hoveredInvIdx = idx;
                break;
            }
        }
    }

    for (int i = 0; i < 8; ++i) {
        const float x = gridLeft + i * (slot + gap);
        if (mouse.x >= x && mouse.x <= x + slot && mouse.y >= hotbarY && mouse.y <= hotbarY + slot) {
            hoveredHotbarIdx = i;
            break;
        }
    }

    const int numKeys[8] = { GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8 };
    for (int i = 0; i < 8; ++i) {
        if (m_input.keyPressed(numKeys[i])) {
            if (hoveredInvIdx >= 0) {
                m_hotbar[i] = m_inventory[hoveredInvIdx];
                m_selectedSlot = i;
            } else if (hoveredHotbarIdx >= 0) {
                std::swap(m_hotbar[i], m_hotbar[hoveredHotbarIdx]);
                m_selectedSlot = i;
            }
        }
    }

    if (m_input.mousePressed(GLFW_MOUSE_BUTTON_LEFT)) {
        if (hoveredInvIdx >= 0) {
            std::swap(m_heldItem, m_inventory[hoveredInvIdx]);
        } else if (hoveredHotbarIdx >= 0) {
            std::swap(m_heldItem, m_hotbar[hoveredHotbarIdx]);
        }
    }
}

// ---------------------------------------------------------------------------
// Gameplay
// ---------------------------------------------------------------------------

void Application::handlePlayInput() {
    if (m_input.keyPressed(GLFW_KEY_E)) {
        openInventory();
        return;
    }
    if (m_input.keyPressed(GLFW_KEY_F))  m_player.toggleFlying();
    if (m_input.keyPressed(GLFW_KEY_F5)) m_player.cyclePerspective();
    if (m_input.keyPressed(GLFW_KEY_G))  m_wireframe = !m_wireframe;

    const int numberKeys[8] = {
        GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4,
        GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8,
    };
    for (int i = 0; i < 8; ++i) {
        if (m_input.keyPressed(numberKeys[i])) m_selectedSlot = i;
    }

    const double scroll = m_input.scrollY();
    if (scroll > 0.0) m_selectedSlot = (m_selectedSlot + 7) % 8;
    else if (scroll < 0.0) m_selectedSlot = (m_selectedSlot + 1) % 8;
}

void Application::updateInteraction() {
    const Camera& camera = m_player.camera();

    if (m_input.cursorCaptured() && m_input.mousePressed(GLFW_MOUSE_BUTTON_LEFT)) {
        m_player.triggerSwing();
        Mob* hitMob = m_entityManager.hitTest(camera.position(), camera.front(), config::REACH_DISTANCE);
        if (hitMob) {
            hitMob->takeDamage(2, m_player.position());
            if (hitMob->type() == MobType::PigmanVillager) {
                m_entityManager.alertNearbyPigmen(hitMob->position(), 16.0f);
            }
            return;
        }
    }

    m_target = raycast(m_world, camera.position(), camera.front(), config::REACH_DISTANCE);

    if (!m_input.cursorCaptured() || !m_target.hit) return;

    if (m_input.mousePressed(GLFW_MOUSE_BUTTON_LEFT)) {
        m_player.triggerSwing();
        const BlockId targetBlock = m_world.getBlock(m_target.block.x, m_target.block.y, m_target.block.z);
        if (isBreakable(targetBlock)) {
            m_world.setBlock(m_target.block.x, m_target.block.y, m_target.block.z, BlockId::Air);
        }
    }

    if (m_input.mousePressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        m_player.triggerSwing();
        const glm::ivec3 place = m_target.block + m_target.normal;
        if (!playerOccupies(place)) {
            BlockId placed = m_hotbar[m_selectedSlot];
            if (placed == BlockId::Wood || placed == BlockId::WoodX || placed == BlockId::WoodZ) {
                if (m_target.normal.x != 0) {
                    placed = BlockId::WoodX;
                } else if (m_target.normal.z != 0) {
                    placed = BlockId::WoodZ;
                } else {
                    placed = BlockId::Wood;
                }
            } else if (isTorch(placed)) {
                if (m_target.normal.x > 0) {
                    placed = BlockId::TorchWallWest; // Attached to West wall
                } else if (m_target.normal.x < 0) {
                    placed = BlockId::TorchWallEast; // Attached to East wall
                } else if (m_target.normal.z > 0) {
                    placed = BlockId::TorchWallNorth; // Attached to North wall
                } else if (m_target.normal.z < 0) {
                    placed = BlockId::TorchWallSouth; // Attached to South wall
                } else {
                    placed = BlockId::Torch; // Floor torch
                }
            }
            m_world.setBlock(place.x, place.y, place.z, placed);
        }
    }
}

bool Application::playerOccupies(const glm::ivec3& block) const {
    const glm::vec3 feet = m_player.position();
    const glm::vec3 minP(feet.x - 0.3f, feet.y, feet.z - 0.3f);
    const glm::vec3 maxP(feet.x + 0.3f, feet.y + 1.8f, feet.z + 0.3f);

    const glm::vec3 bMin(block);
    const glm::vec3 bMax = bMin + glm::vec3(1.0f);

    return minP.x < bMax.x && maxP.x > bMin.x &&
           minP.y < bMax.y && maxP.y > bMin.y &&
           minP.z < bMax.z && maxP.z > bMin.z;
}

void Application::rebuildDirtyMeshes() {
    int budget = 16; // Rebuild up to 16 chunks/frame prioritizing nearest to camera
    const glm::vec3 camPos = (m_state == GameState::MainMenu || m_state == GameState::SelectWorld ||
                              m_state == GameState::NewWorld || m_state == GameState::DeleteWorld ||
                              m_state == GameState::ConfirmDeleteWorld)
                             ? m_menuCamera.position()
                             : m_player.camera().position();

    struct DistanceChunk {
        Chunk* chunk;
        float distSq;
    };
    std::vector<DistanceChunk> dirtyChunks;
    dirtyChunks.reserve(64);

    for (const std::unique_ptr<Chunk>& chunk : m_world.chunks()) {
        if (!chunk || !chunk->dirty) continue;
        const float cx = static_cast<float>(chunk->originX() + Chunk::W / 2);
        const float cz = static_cast<float>(chunk->originZ() + Chunk::D / 2);
        const float dx = cx - camPos.x;
        const float dz = cz - camPos.z;
        dirtyChunks.push_back({ chunk.get(), dx * dx + dz * dz });
    }

    if (dirtyChunks.empty()) return;

    std::sort(dirtyChunks.begin(), dirtyChunks.end(), [](const DistanceChunk& a, const DistanceChunk& b) {
        return a.distSq < b.distSq;
    });

    for (const auto& dc : dirtyChunks) {
        if (budget <= 0) break;
        Chunk* chunk = dc.chunk;
        std::vector<Vertex> opVertices, trVertices;
        std::vector<uint32_t> opIndices, trIndices;
        buildChunkGeometry(m_world, *chunk, opVertices, opIndices, trVertices, trIndices);
        chunk->mesh.upload(opVertices, opIndices);
        chunk->transparentMesh.upload(trVertices, trIndices);
        chunk->dirty = false;
        --budget;
    }
}

// ---------------------------------------------------------------------------
// Settings / platform
// ---------------------------------------------------------------------------

void Application::applySettings() {
    m_player.setMouseSensitivity(m_mouseSensitivity);
    m_player.camera().setFov(m_fov);
    m_menuCamera.setFov(m_fov);
    glfwSwapInterval(m_vsync ? 1 : 0);
}

float Application::computeUiScale() const {
    if (m_guiScale > 0) return static_cast<float>(m_guiScale);

    // Auto: pick an integer scale from the smaller screen dimension so the UI
    // keeps a consistent apparent size and does not balloon in fullscreen.
    const float height = static_cast<float>(m_fbHeight);
    const float width = static_cast<float>(m_fbWidth);
    const float metric = std::min(height, width * 0.6f);
    long scale = std::lround(metric / 280.0f);
    scale = std::clamp<long>(scale, 1, 8);
    return static_cast<float>(scale);
}

void Application::setFullscreen(bool enabled) {
    if (enabled == m_fullscreen) return;
    m_fullscreen = enabled;

    if (enabled) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        glfwSetWindowMonitor(m_window, nullptr, 100, 100,
                             config::WINDOW_WIDTH, config::WINDOW_HEIGHT, 0);
    }
}

void Application::setCursorCaptured(bool captured) {
    m_input.setCursorCaptured(captured);
    glfwSetInputMode(m_window, GLFW_CURSOR,
                     captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    m_input.resetMouseTracking();
}

glm::vec2 Application::mouseInFramebuffer() const {
    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(m_window, &mouseX, &mouseY);

    int windowWidth = 1;
    int windowHeight = 1;
    glfwGetWindowSize(m_window, &windowWidth, &windowHeight);
    if (windowWidth <= 0 || windowHeight <= 0) return glm::vec2(0.0f);

    const float scaleX = static_cast<float>(m_fbWidth) / static_cast<float>(windowWidth);
    const float scaleY = static_cast<float>(m_fbHeight) / static_cast<float>(windowHeight);
    return glm::vec2(static_cast<float>(mouseX) * scaleX,
                     static_cast<float>(m_fbHeight) - static_cast<float>(mouseY) * scaleY);
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void Application::renderLoadingScreen(const std::string& title, float progress, const std::string& statusMessage) {
    if (!m_window) return;

    glfwGetFramebufferSize(m_window, &m_fbWidth, &m_fbHeight);
    m_renderer.setViewport(m_fbWidth, m_fbHeight);
    m_renderer.setUIScale(computeUiScale());

    m_renderer.beginFrame(glm::vec3(0.08f, 0.09f, 0.12f));
    m_renderer.beginUI();

    const float fw = static_cast<float>(m_fbWidth);
    const float fh = static_cast<float>(m_fbHeight);
    const float s = m_renderer.uiScale();

    // 1. Full-screen obsidian-gradient backdrop
    m_renderer.drawGradientRect(0.0f, 0.0f, fw, fh,
                                glm::vec4(0.05f, 0.06f, 0.08f, 1.0f),
                                glm::vec4(0.12f, 0.15f, 0.20f, 1.0f));

    // 2. Centered dialog box
    const float cardW = std::min(fw * 0.85f, 480.0f * s);
    const float cardH = 175.0f * s;
    const float cardX = (fw - cardW) * 0.5f;
    const float cardY = (fh - cardH) * 0.5f;

    // Outer border & shadow
    m_renderer.drawRect(cardX - 4.0f * s, cardY - 4.0f * s, cardW + 8.0f * s, cardH + 8.0f * s,
                        glm::vec4(0.0f, 0.0f, 0.0f, 0.65f));
    m_renderer.drawRect(cardX - 2.0f * s, cardY - 2.0f * s, cardW + 4.0f * s, cardH + 4.0f * s,
                        glm::vec4(0.28f, 0.32f, 0.40f, 1.0f));
    m_renderer.drawGradientRect(cardX, cardY, cardW, cardH,
                                glm::vec4(0.10f, 0.11f, 0.14f, 0.95f),
                                glm::vec4(0.16f, 0.18f, 0.23f, 0.95f));

    // Top gold highlight accent line
    m_renderer.drawRect(cardX, cardY + cardH - 3.0f * s, cardW, 3.0f * s,
                        glm::vec4(0.85f, 0.72f, 0.22f, 1.0f));

    // 3. Header text: "BUILDING WORLD"
    const std::string header = "BUILDING WORLD";
    const float hScale = s * 1.6f;
    const float hW = m_renderer.textWidth(header, hScale);
    m_renderer.drawText((fw - hW) * 0.5f + 1.0f * s, cardY + cardH - 32.0f * s - 1.0f * s,
                        header, hScale, glm::vec4(0.0f, 0.0f, 0.0f, 0.7f));
    m_renderer.drawText((fw - hW) * 0.5f, cardY + cardH - 32.0f * s,
                        header, hScale, glm::vec4(1.0f, 0.85f, 0.28f, 1.0f));

    // 4. World Name
    const std::string worldText = "World: " + title;
    const float wScale = s * 1.05f;
    const float wW = m_renderer.textWidth(worldText, wScale);
    m_renderer.drawText((fw - wW) * 0.5f, cardY + cardH - 54.0f * s,
                        worldText, wScale, glm::vec4(0.85f, 0.88f, 0.92f, 1.0f));

    // 5. Progress Bar
    const float barW = cardW - 50.0f * s;
    const float barH = 20.0f * s;
    const float barX = cardX + 25.0f * s;
    const float barY = cardY + 58.0f * s;

    // Bar background trough
    m_renderer.drawRect(barX - 2.0f * s, barY - 2.0f * s, barW + 4.0f * s, barH + 4.0f * s,
                        glm::vec4(0.04f, 0.05f, 0.06f, 1.0f));
    m_renderer.drawRect(barX - 1.0f * s, barY - 1.0f * s, barW + 2.0f * s, barH + 2.0f * s,
                        glm::vec4(0.35f, 0.38f, 0.45f, 1.0f));
    m_renderer.drawRect(barX, barY, barW, barH, glm::vec4(0.08f, 0.09f, 0.11f, 1.0f));

    // Filled progress bar
    float clampedProgress = std::clamp(progress, 0.02f, 1.0f);
    const float fillW = (barW - 4.0f * s) * clampedProgress;
    if (fillW > 0.0f) {
        m_renderer.drawGradientRect(barX + 2.0f * s, barY + 2.0f * s, fillW, barH - 4.0f * s,
                                    glm::vec4(0.20f, 0.65f, 0.25f, 1.0f),
                                    glm::vec4(0.45f, 0.88f, 0.35f, 1.0f));
    }

    // Percentage Text (centered over bar)
    int pct = static_cast<int>(clampedProgress * 100.0f + 0.5f);
    char pctBuf[32];
    std::snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
    const std::string pctStr(pctBuf);
    const float pctScale = s * 0.95f;
    const float pctW = m_renderer.textWidth(pctStr, pctScale);
    m_renderer.drawText((fw - pctW) * 0.5f + 1.0f * s, barY + (barH - m_renderer.textHeight(pctScale)) * 0.5f - 1.0f * s,
                        pctStr, pctScale, glm::vec4(0.0f, 0.0f, 0.0f, 0.9f));
    m_renderer.drawText((fw - pctW) * 0.5f, barY + (barH - m_renderer.textHeight(pctScale)) * 0.5f,
                        pctStr, pctScale, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // 6. Status Message
    const float msgScale = s * 0.95f;
    const float msgW = m_renderer.textWidth(statusMessage, msgScale);
    m_renderer.drawText((fw - msgW) * 0.5f, cardY + 30.0f * s,
                        statusMessage, msgScale, glm::vec4(0.40f, 0.80f, 0.95f, 1.0f));

    // 7. Tip / hint
    const std::string tip = "Tip: Pigmen villagers will defend their settlement if attacked!";
    const float tipScale = s * 0.80f;
    const float tipW = m_renderer.textWidth(tip, tipScale);
    m_renderer.drawText((fw - tipW) * 0.5f, cardY + 10.0f * s,
                        tip, tipScale, glm::vec4(0.60f, 0.64f, 0.70f, 1.0f));

    m_renderer.endUI();
    glfwSwapBuffers(m_window);
    glfwPollEvents();
}

void Application::render() {
    renderScene();
    drawDebugOverlayIfEnabled();
}

void Application::drawDebugOverlayIfEnabled() {
    if (!m_showDebugOverlay) return;

    const FrameStats& s = m_renderer.stats();
    const glm::vec3 eye = m_player.eyePosition();

    char buffer[128];
    std::vector<std::string> lines;

    std::snprintf(buffer, sizeof(buffer), "VoxelMin v%s  %dx%d", config::VERSION, m_fbWidth, m_fbHeight);
    lines.emplace_back(buffer);

    std::snprintf(buffer, sizeof(buffer), "frame %.2f ms   %.0f fps   (CPU, 90-frame avg)", s.avgFrameMs, s.avgFps);
    lines.emplace_back(buffer);

    std::snprintf(buffer, sizeof(buffer), "draw calls %.1f avg / %d now", s.avgDrawCalls, s.drawCalls);
    lines.emplace_back(buffer);

    std::snprintf(buffer, sizeof(buffer), "triangles %.0f avg", s.avgTriangles);
    lines.emplace_back(buffer);

    std::snprintf(buffer, sizeof(buffer), "chunks %zu total, %.1f drawn, %.1f visible, %.1f culled",
                  m_world.chunks().size(), s.avgChunksDrawn, s.avgChunksVisible, s.avgChunksCulled);
    lines.emplace_back(buffer);

    std::snprintf(buffer, sizeof(buffer), "mobs %zu   particles %zu",
                  m_entityManager.mobs().size(), m_renderer.particleCount());
    lines.emplace_back(buffer);

    std::snprintf(buffer, sizeof(buffer), "xyz %.1f %.1f %.1f   yaw %.0f",
                  eye.x, eye.y, eye.z, m_player.yaw());
    lines.emplace_back(buffer);

    m_renderer.drawDebugOverlay(lines);
}

void Application::renderScene() {
    const bool pausedBackground =
        (m_state == GameState::Paused) ||
        (m_state == GameState::Options && m_optionsReturn == GameState::Paused);
    const Camera& camera = pausedBackground ? m_player.camera() : m_menuCamera;

    const glm::vec3 eye = (m_state == GameState::Playing || m_state == GameState::Inventory || pausedBackground)
                          ? m_player.eyePosition() : m_menuCamera.position();
    const int ex = static_cast<int>(std::floor(eye.x));
    const int ey = static_cast<int>(std::floor(eye.y));
    const int ez = static_cast<int>(std::floor(eye.z));
    const bool underwater = isLiquid(m_world.getBlock(ex, ey, ez));

    const glm::vec3 skyColor = underwater ? glm::vec3(0.06f, 0.18f, 0.44f) : glm::vec3(0.54f, 0.72f, 0.98f);
    const float fogEnd = underwater ? 15.0f : static_cast<float>(m_viewDistanceChunks) * 16.0f;
    const float fogStart = underwater ? 1.0f : fogEnd * 0.45f;

    m_renderer.setUIScale(computeUiScale());
    m_renderer.beginFrame(skyColor);

    if (m_state == GameState::Playing) {
        glPolygonMode(GL_FRONT_AND_BACK, m_wireframe ? GL_LINE : GL_FILL);
        m_renderer.drawWorld(m_world, m_player.camera(), skyColor, fogStart, fogEnd);
        m_renderer.drawEntities(m_entityManager, m_world, m_player.camera(), skyColor, fogStart, fogEnd);
        m_renderer.drawPlayer(m_player, m_world, m_player.camera(), skyColor, fogStart, fogEnd);
        m_renderer.drawFirstPersonArm(m_player, m_world, m_player.camera());
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        if (m_target.hit) {
            const BlockId targetBlock = m_world.getBlock(m_target.block.x, m_target.block.y, m_target.block.z);
            m_renderer.drawSelection(m_player.camera(), m_target.block, targetBlock);
        }

        if (underwater) {
            m_renderer.drawUnderwaterOverlay(static_cast<float>(m_uiTime));
        }

        m_renderer.drawHud(m_selectedSlot, m_hotbar, 8);
        return;
    }

    if (m_state == GameState::Inventory) {
        m_renderer.drawWorld(m_world, m_player.camera(), skyColor, fogStart, fogEnd);
        m_renderer.drawEntities(m_entityManager, m_world, m_player.camera(), skyColor, fogStart, fogEnd);
        m_renderer.drawPlayer(m_player, m_world, m_player.camera(), skyColor, fogStart, fogEnd);
        m_renderer.drawFirstPersonArm(m_player, m_world, m_player.camera());
        if (underwater) {
            m_renderer.drawUnderwaterOverlay(static_cast<float>(m_uiTime));
        }
        m_renderer.beginUI();
        m_renderer.drawRect(0.0f, 0.0f, static_cast<float>(m_fbWidth), static_cast<float>(m_fbHeight),
                            glm::vec4(0.0f, 0.0f, 0.0f, 0.60f));
        m_renderer.endUI();
        m_renderer.drawInventory(m_selectedSlot, m_hotbar, 8, m_inventory, 24, m_heldItem, mouseInFramebuffer());
        return;
    }

    m_renderer.drawWorld(m_world, camera, skyColor, fogStart, fogEnd);
    m_renderer.drawEntities(m_entityManager, m_world, camera, skyColor, fogStart, fogEnd);
    if (pausedBackground) {
        m_renderer.drawPlayer(m_player, m_world, camera, skyColor, fogStart, fogEnd);
        m_renderer.drawFirstPersonArm(m_player, m_world, camera);
    }
    if (underwater && pausedBackground) {
        m_renderer.drawUnderwaterOverlay(static_cast<float>(m_uiTime));
    }

    m_renderer.beginUI();
    const float width = static_cast<float>(m_fbWidth);
    const float height = static_cast<float>(m_fbHeight);
    if (pausedBackground) {
        m_renderer.drawRect(0.0f, 0.0f, width, height, glm::vec4(0.0f, 0.0f, 0.0f, 0.55f));
    } else {
        m_renderer.drawGradientRect(0.0f, height * 0.55f, width, height * 0.45f,
                                    glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
                                    glm::vec4(0.0f, 0.0f, 0.0f, 0.55f));
        m_renderer.drawGradientRect(0.0f, 0.0f, width, height * 0.45f,
                                    glm::vec4(0.0f, 0.0f, 0.0f, 0.65f),
                                    glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    }
    m_renderer.endUI();

    activeMenu().render(m_renderer, m_uiTime);
}

// ---------------------------------------------------------------------------
// Headless capture (optional, environment driven)
// ---------------------------------------------------------------------------

void Application::applyCaptureEnvironment() {
    if (const char* path = std::getenv("VOXELMIN_CAPTURE")) m_capturePath = path;
    if (m_capturePath.empty()) return;

    if (const char* frames = std::getenv("VOXELMIN_CAPTURE_FRAMES")) {
        const int parsed = std::atoi(frames);
        if (parsed > 0) m_captureFrame = parsed;
    }

    if (const char* size = std::getenv("VOXELMIN_CAPTURE_SIZE")) {
        int w = 0;
        int h = 0;
        if (std::sscanf(size, "%dx%d", &w, &h) == 2 && w > 0 && h > 0) {
            glfwSetWindowSize(m_window, w, h);
        }
    }

    if (const char* gui = std::getenv("VOXELMIN_CAPTURE_GUI_SCALE")) {
        const int parsed = std::atoi(gui);
        if (parsed >= 0 && parsed <= 8) m_guiScale = parsed;
    }

    // Force the F3 overlay on for captures, so a screenshot can prove the
    // overlay renders correctly.
    if (const char* dbg = std::getenv("VOXELMIN_CAPTURE_DEBUG")) {
        if (std::atoi(dbg) != 0) m_showDebugOverlay = true;
    }

    // VSync pins the measured frame rate to the display refresh (165 Hz on the
    // dev machine), which hides the real frame cost. Captures can turn it off to
    // get an uncapped number for the metrics table.
    if (const char* vs = std::getenv("VOXELMIN_CAPTURE_VSYNC")) {
        m_vsync = (std::atoi(vs) != 0);
        glfwSwapInterval(m_vsync ? 1 : 0);
    }

    const char* stateEnv = std::getenv("VOXELMIN_CAPTURE_STATE");
    const std::string state = stateEnv ? stateEnv : "";
    if (state == "play") {
        startNewWorld(m_activeWorldName, m_activeWorldSeed);
    } else if (state == "select_world") {
        openSelectWorld();
    } else if (state == "new_world") {
        openNewWorld();
    } else if (state == "pause") {
        startNewWorld(m_activeWorldName, m_activeWorldSeed);
        m_capturePauseAt = 10; // let gameplay update the camera first
    } else if (state == "options") {
        openOptions(GameState::MainMenu);
    } else if (state == "pause_options") {
        startNewWorld(m_activeWorldName, m_activeWorldSeed);
        m_capturePauseAt = 10;
        m_captureOptionsAt = 20;
    }

    if (const char* camEnv = std::getenv("VOXELMIN_CAPTURE_CAM")) {
        float cx = 0.0f, cy = 0.0f, cz = 0.0f, cyaw = -90.0f, cpitch = -10.0f;
        if (std::sscanf(camEnv, "%f,%f,%f,%f,%f", &cx, &cy, &cz, &cyaw, &cpitch) == 5) {
            m_player.setPosition({cx, cy, cz});
            m_player.setRotation(cyaw, cpitch);
            m_player.setFlying(true);
        }
    }

    if (const char* perspEnv = std::getenv("VOXELMIN_CAPTURE_PERSPECTIVE")) {
        const std::string p(perspEnv);
        if (p == "third_back" || p == "third" || p == "back") {
            m_player.setPerspective(Perspective::ThirdPersonBack);
        } else if (p == "third_front" || p == "front") {
            m_player.setPerspective(Perspective::ThirdPersonFront);
        } else {
            m_player.setPerspective(Perspective::FirstPerson);
        }
    }

    if (const char* torchEnv = std::getenv("VOXELMIN_CAPTURE_TORCH")) {
        int tx = 0, ty = 0, tz = 0;
        if (std::sscanf(torchEnv, "%d,%d,%d", &tx, &ty, &tz) == 3) {
            m_world.setBlock(tx, ty - 1, tz, BlockId::Stone);
            m_world.setBlock(tx, ty, tz, BlockId::Torch);
        }
    }

    if (const char* wallTorchEnv = std::getenv("VOXELMIN_CAPTURE_WALL_TORCH")) {
        int tx = 0, ty = 0, tz = 0;
        char side[16] = {0};
        if (std::sscanf(wallTorchEnv, "%d,%d,%d,%15s", &tx, &ty, &tz, side) >= 3) {
            std::string s = side;
            if (s == "west") {
                m_world.setBlock(tx + 1, ty, tz, BlockId::Cobblestone);
                m_world.setBlock(tx, ty, tz, BlockId::TorchWallWest);
            } else if (s == "south") {
                m_world.setBlock(tx, ty, tz + 1, BlockId::Cobblestone);
                m_world.setBlock(tx, ty, tz, BlockId::TorchWallSouth);
            } else if (s == "north") {
                m_world.setBlock(tx, ty, tz - 1, BlockId::Cobblestone);
                m_world.setBlock(tx, ty, tz, BlockId::TorchWallNorth);
            } else { // default east
                m_world.setBlock(tx - 1, ty, tz, BlockId::Cobblestone);
                m_world.setBlock(tx, ty, tz, BlockId::TorchWallEast);
            }
        }
    }

    if (const char* waterEnv = std::getenv("VOXELMIN_CAPTURE_WATER")) {
        int wx = 0, wy = 0, wz = 0;
        if (std::sscanf(waterEnv, "%d,%d,%d", &wx, &wy, &wz) == 3) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -2; dz <= 2; ++dz) {
                    for (int dx = -2; dx <= 2; ++dx) {
                        m_world.setBlock(wx + dx, wy + dy, wz + dz, BlockId::Water);
                    }
                }
            }
        }
    }
}

void Application::maybeCapture() {
    if (m_capturePath.empty() || m_captureDone) return;
    ++m_frameCounter;

    if (m_capturePauseAt >= 0 && m_frameCounter >= m_capturePauseAt &&
        m_state == GameState::Playing) {
        pauseGame();
    }
    if (m_captureOptionsAt >= 0 && m_frameCounter >= m_captureOptionsAt &&
        m_state == GameState::Paused) {
        openOptions(GameState::Paused);
    }

    if (m_frameCounter < m_captureFrame) return;
    m_captureDone = true;

    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    std::vector<uint8_t> pixels(static_cast<size_t>(m_fbWidth) * static_cast<size_t>(m_fbHeight) * 3);
    glReadPixels(0, 0, m_fbWidth, m_fbHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    writeBmp(m_capturePath, m_fbWidth, m_fbHeight, pixels);
    log::info("Captured frame to %s (%dx%d)", m_capturePath.c_str(), m_fbWidth, m_fbHeight);

    // Log the render statistics alongside the capture so the numbers in
    // docs/PROGRESS.md can be measured without a human watching the overlay.
    const FrameStats& s = m_renderer.stats();
    log::info("Stats: %.1f fps, %.2f ms/frame, %.1f draw calls, %.0f triangles, "
              "%.1f chunks drawn, %.1f visible, %.1f culled, %zu mobs, %zu particles",
              s.avgFps, s.avgFrameMs, s.avgDrawCalls, s.avgTriangles,
              s.avgChunksDrawn, s.avgChunksVisible, s.avgChunksCulled,
              m_entityManager.mobs().size(), m_renderer.particleCount());

    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void Application::writeBmp(const std::string& path, int width, int height,
                           const std::vector<uint8_t>& rgb) const {
    const int rowSize = (width * 3 + 3) & ~3;
    const int dataSize = rowSize * height;
    const int fileSize = 54 + dataSize;
    std::vector<uint8_t> bmp(static_cast<size_t>(fileSize), 0);

    const auto put16 = [&](int offset, uint16_t value) {
        bmp[static_cast<size_t>(offset)] = static_cast<uint8_t>(value & 0xFF);
        bmp[static_cast<size_t>(offset) + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    };
    const auto put32 = [&](int offset, uint32_t value) {
        for (int i = 0; i < 4; ++i) {
            bmp[static_cast<size_t>(offset) + static_cast<size_t>(i)] =
                static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
        }
    };

    bmp[0] = 'B';
    bmp[1] = 'M';
    put32(2, static_cast<uint32_t>(fileSize));
    put32(10, 54);
    put32(14, 40);
    put32(18, static_cast<uint32_t>(width));
    put32(22, static_cast<uint32_t>(height));
    put16(26, 1);
    put16(28, 24);
    put32(34, static_cast<uint32_t>(dataSize));

    // glReadPixels returns bottom-up RGB, which matches BMP's row order; only
    // the channel order differs (BMP is BGR).
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const uint8_t* src = &rgb[(static_cast<size_t>(y) * width + x) * 3];
            uint8_t* dst = &bmp[static_cast<size_t>(54 + y * rowSize + x * 3)];
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
        }
    }

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        log::error("Failed to open capture file: %s", path.c_str());
        return;
    }
    file.write(reinterpret_cast<const char*>(bmp.data()), static_cast<std::streamsize>(bmp.size()));
}

// ---------------------------------------------------------------------------
// GLFW callbacks
// ---------------------------------------------------------------------------

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->m_input.onKey(key, scancode, action, mods);
}

void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->m_input.onMouseButton(button, action, mods);
}

void Application::cursorPosCallback(GLFWwindow* window, double x, double y) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->m_input.onCursorPos(x, y);
}

void Application::scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->m_input.onScroll(xOffset, yOffset);
}

void Application::charCallback(GLFWwindow* window, unsigned int codepoint) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->handleCharInput(codepoint);
}

} // namespace vox

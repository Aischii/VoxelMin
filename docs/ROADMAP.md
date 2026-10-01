# Roadmap

Ordered milestones. Each is a coherent, shippable increment. Update
`docs/PROGRESS.md` when a milestone completes.

## M1 -- Walk, look, dig, build -- DONE

Window, chunked world, procedural terrain, culled meshing, physics, raycast
interaction, hotbar, fog.

## M2 -- Menus & UI (Legacy-console style) -- DONE

- Main menu with a rotating world panorama, logo, green underline, splash text
  and footer control hints.
- In-game pause menu ("Game Menu") over a dimmed, frozen world.
- Options screen with live settings (mouse sensitivity, FOV, view distance,
  VSync, fullscreen, wireframe), navigable by keyboard and mouse.
- Procedural 5x7 bitmap font (no font assets) and a `Renderer` 2D primitive API.
- Environment-driven screenshot capture for verification.

Follow-ups: D-pad/controller support, key rebinding, menu sounds, locale text.

## M3 -- Playability polish -- DONE

- **Greedy meshing** to merge coplanar faces (big vertex-count reduction) with hardware texture gradient filtering.
- **Frustum culling** so off-screen chunks are skipped via Gribb-Hartmann extraction.
- **Transparency pass**: dual-pass geometry (opaque pass + alpha cutout transparent pass for leaves).
- **Save/load** the world (`world.dat` VOXS v1 binary format with RLE chunk compression) and remember the player.
- Authentic "Save and Quit to Title" in-game pause action with persistence.
- External randomized splash text system (`assets/splashes.txt`).
- 32x32 window application icon.

## M4 -- World Depth & Lighting -- DONE

- **Lighting**: Skylight column propagation with horizontal diffusion + smooth 4-level vertex Ambient Occlusion (`computeVertexAO`).
- **Procedural Trees**: Multi-archetype randomized tree styles (classic oak, tall forest oak, conical pine/spruce, bushy tree) with full canopy coverage.
- **Seeded World Types & Named Saves**: Dedicated Seed generation, Preset types, and named persistence (`saves/<world_name>.dat`).
- **Caves & Mineral Ores**: 3D noise cavern carving with embedded Coal, Iron, Gold, and Diamond veins.
- **Water & Beaches**: Sea level water basins with sandy shorelines and transparent pass rendering.
- **Torches & Point Block Lighting**: Placeable torches with dynamic 3D light propagation up to 7 blocks and immediate mesh rebuilds.

## M5 -- Gameplay & Entities -- DONE

- **Creative / Storage Inventory (Done)**: 24-slot Storage Inventory modal + Hotbar with 16x16 textured item sprites and drag-and-drop.
- **Player Sprinting & Dynamic FOV (Done)**: Sprinting via `Left Control` / double-tap `W` (7.4 blocks/s) and smooth FOV dynamic zoom.
- **Neutral Mobs & AI (Done)**: Autonomous Pigs and Cows with animated walking legs, idle looking, player curiosity, panic behaviors on hit, and full voxel physics.
- **Neutral Pigman Villagers & Village Defense (Done)**: Bipedal Pigman Villagers who wander their village peacefully, group-retaliate when attacked, and leash back to their village when enemies flee.
- **Procedural Multi-Template Surface Villages (Done)**: 4 distinct village layout archetypes (Crossroads, High Street, Ring Commons, Fortified Enclave) with surface foundations, wells, cottages, farmlands, and torches.
- **World Scale Expansion to 512x512 (Done)**: 1,024 chunks ($32 \times 32$) with camera-distance prioritized chunk meshing and extended fog horizons.
- **Batched Mob Rendering (Done)**: Dynamic streaming entity buffer sharing world shaders, lighting, AO, and fog.
- **3D Torches & Particle System (Done)**: 3D standing wooden torch sticks with dynamic billboard flame/smoke particle effects.
- **Water Mechanics & Protection (Done)**: Non-breakable fluid raycast pass-through and recessed liquid surface meshing.
- **World Types & Interactive Name Entry (Done)**: 5 generation types (`Default`, `Flat`, `Mountainous`, `Cavernous`, `Island`) and keyboard text editing.
- **Sub-Block Precision Raycasting & Torch Hitbox (Done)**: Compact $0.25 \times 0.65 \times 0.25$ AABB selection wireframes and ray pass-through.
- **Wild Tall Grass Foliage (Done)**: Procedural cross-quad foliage meshing in cutout transparent pass with anti-flicker stability.
- **World Deletion Management (Done)**: World deletion with confirmation modal dialogs and `Del` key shortcut on the Select World menu.
- **Procedural Menu Panorama & Zero-Stutter World Startup (Done)**: Lightweight $128 \times 128$ main menu panorama and live chunk pre-meshing on world generation / loading screen.
- **Player Model (Steve) & Minecraft Biped Anatomy (Done)**: Standard biped proportions, hair/face/tshirt/pants/shoes textures, head pitch articulation, and swinging limbs.
- **Camera-Space Locked Viewmodel & Pure First-Person Hand (Done)**: Screen-anchored right arm viewmodel with identity view matrix projection and clean first-person rendering without body clipping.
- **Multi-Directional 3D Wall Torches (Done)**: Angled wall torch placement (West, East, North, South) with slanted 3D geometry, custom hitboxes, and particle emission.
- **F5 Third-Person Perspectives & Anti-Clip Camera (Done)**: Front and back third-person camera modes with anti-clip terrain raycasting.
- **Mob Knockback Physics & Aggro Interest Timers (Done)**: Impulse recoil flinch, hitstun state, interest timeouts, and robust multi-biome wild animal spawning.
- **Item entities & drops (Done)**: 3D bobbing/spinning mini-cubes, gravity and voxel collision physics, magnetic vacuum pickup, block break item drops.
- **Crafting & Tools (Done)**: 2x2 crafting grid, authentic recipes (wood, planks, sticks, crafting table, torches, pickaxes, axes, shovels, swords), durability bars, and mining/attack damage multipliers.

**Milestone closed with release v0.M5.6.**

## M6 -- Audio & Save Persistence (DONE)

- **Audio Engine Core**: Procedural 44.1kHz stereo audio engine powered by `miniaudio`, zero external sound asset dependencies.
- **Sound Banks & Spatial 3D Audio**: Footsteps, block dig/place/break, item pickups, mob hit/grunt, tool break, and crisp UI clicks with distance attenuation & stereo panning.
- **Background Music (BGM)**: Multi-format disk track streaming (`.mp3`, `.wav`, `.flac`, `.ogg`) from `assets/music/` and `assets/audio/`, with 16-second procedural ambient chord loop synthesizer fallback.
- **Future-Proof Saves & Protection**: Persistent project-level `saves/` storage, automated clean-script preservation, legacy folder auto-migration, and forward-compatible block ID sanitization.

## M7 -- Lighting & Visual Immersion (DONE)

- **4-Corner Vertex Ambient Occlusion (AO)**: Smooth 4-level corner occlusion shading per quad vertex, adding authentic depth to block corners and crevices.
- **BFS Flood-Fill Light Engine**: Nibble-packed Sunlight (0–15) and Blocklight (0–15) per voxel with breadth-first search light propagation and dynamic torch place/break updates.
- **True Darkness & Celestial Cycle**: Pitch-black natural night and unlit caves, sun/moon orbits, and dynamic sky gradient.
- **Separate Menu & Exploration Audio**: Distinct procedural title arpeggio and in-game exploration pads with smooth 1-second dynamic crossfading.

## M8 -- Console Crafting, RPG HUD & Ecological Survival (DONE)

- **Console Edition Crafting UI**: Legacy Console Edition category tabs (`Structures`, `Tools & Weapons`, `Food & Essentials`, `Mechanisms`, `Decorations`), horizontal recipe selector, visual ingredient preview, and direct 1-click crafting.
- **Interactive 3x3 Crafting Table**: Right-clicking a placed Crafting Table opens full 3x3 workbench crafting with 3x3 recipes (Tools, Weapons, Armor, Mechanisms).
- **RPG MMO Vitals Card HUD**: Top-left player status card with Steve portrait, Level badge, vertical XP bar, nameplate, red health bar, and Food, Oxygen, and Armor stats with `+20` sub-counters.
- **8-Phase Lunar Light Cycle**: Authentic lunar cycle ($0..7$) where Full Moon emits soft blue ambient moonlight (`0.065f`) and New Moon plunges the world into pure pitch-black night (`0.002f`).
- **World Ecology Simulation**: Unsupported tall grass instantly collapses when the ground below is destroyed, and dirt exposed to open sky naturally converts to lush grass blocks over time.
- **Item Drops & Physics Toss**: 5-minute item despawn, 1.6m magnetic draw range, 0.95m pickup range, 'Q' key for dropping 1 item and Ctrl+'Q' for tossing full stack with forward trajectory impulse and pickup cooldown.
- **First-Person Held Items**: Camera-locked right arm viewmodel dynamically holding 3D mini-blocks or 2.5D tool sprites with bobbing and swing animations.
- **Creative Enhancements**: Automatic absorption/clearing of full inventory pickups in creative mode, and double-space / double-jump flight toggle.
- **Non-Freezing Death Screen**: Mobs, items, particles, and time continue simulating uninterrupted behind the Game Over screen.
- **Multi-Path World Save Deletion**: Clean removal of saved worlds across all search paths.

**Milestone closed with release v0.M8.0.**

## M9 -- Async World Streaming & ChunkSections (Planned / Next)

- **16x16x16 Sub-Chunk Architecture (ChunkSections)**: Vertical subdivision of chunks into 16x16x16 sections, speeding up block edits $5\times$, dropping empty air sections from GPU RAM, and preparing for expanded world heights ($Y=256$).
- **Multi-Threaded Chunk Worker Pool**: Background worker threads running terrain noise and mesh calculations off-thread; main thread only performs fast sub-millisecond GPU uploads.
- **Dynamic Render Distance Radius**: Configurable render radius (4 to 16 chunks) with ring loading and unloader queue.
- **Stutter-Free Traversal**: Sustained 60+ FPS when sprinting/flying across 1,000+ blocks in any direction.

## M10 -- Survival Hostile Entities & Smelting

- **Hostile Mobs & Night Spawning**: Zombies and Skeletons spawning at low light levels (Light $< 7$), with 3D pathfinding and melee combat attacks.
- **Furnace & Smelting System**: 3-slot Furnace UI (Input, Fuel, Output) with burning animation timer for smelting raw ores and cooking meats.
- **Beds & Day-Skipping**: Craftable wooden beds enabling sleeping through the dangerous night cycle.

## M10 -- Biomes, Weather & World Archetypes

- **Multi-Biome Terrain Generation**: Continuous 2D biome maps (Plains, Deserts with cacti, Snowy Taiga with frosted leaves, Mountains with steep cliff faces).
- **Dynamic Weather System**: Rainfall and Snowstorms with localized particle emitters, overcast cloud cover, and altered sound ambiance.
- **Underground Dungeons**: Cobblestone dungeon rooms with loot chests and spawner cages buried deep within cavern systems.

## Backlog (unscoped)

Ideas with no milestone attached. Pick freely if you want to.

- Seed/world-size entry on the world-creation screen (today these are in
  `src/core/Config.hpp`).
- Alternate dimensions.
- Controller / D-pad menu navigation and key rebinding.

## See also

- [`PROGRESS.md`](PROGRESS.md) -- living status, version history, metrics, open
  decisions, verification and regression checklists.
- [`ARCHITECTURE.md`](ARCHITECTURE.md) -- how the code fits together.
- [`SOURCES.md`](SOURCES.md) -- external references and library choices.
- [`BUILD.md`](BUILD.md) -- toolchain setup and build commands.
- [`HANDOFF.md`](HANDOFF.md) -- notes for a future maintainer.

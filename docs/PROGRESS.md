# Progress

Living status document. Update this whenever behaviour changes. Newest entries
first. Format: `YYYY-MM-DD -- summary`.

## Version scheme

Versions are **milestone-tagged**: `v<major>.M<milestone>.<bump>`. The milestone part
ties each version to `docs/ROADMAP.md`, so "M1" and "v1.M1.x" can never drift
apart, and a version number never reads as a percentage of Minecraft.

Releases before this scheme used ad-hoc `v0.N.P` tags. History is preserved in
the table below rather than rewritten.

## Version history

| Version | Legacy tag | Milestone | Headline |
|---------|------------|-----------|----------|
| v1.M1.1 | - | v1.M1 | Complete Environmental Soundscapes (Wind, Crickets, Cave Drone), Crisp High-Res Vector Font Rasterization & Trilinear Mipmapping, Non-Auto Flight Creative Mode, Dynamic Bit-Packed Chunk Palettes |
| v1.M1.0 | - | v1.M1 | Environmental Sound Physics & Spatial Acoustics, Infinite World Generation & Dynamic Chunk Streaming, Fast Spawn Generation ($<150\text{ ms}$), 4-Corner Per-Vertex Smooth Lighting |
| v0.M9.0 | - | v0.M9 | 4-Corner Per-Vertex Smooth Dynamic Lighting, Trilinear Entity Lighting, Recipe Discovery Progression, Interactive World Seed Input & Hashing, Third-Person Head Pitch Articulation, Distinct 3x3 Workbench UI |
| v0.M8.0 | - | v0.M8 | Console Edition Crafting UI, RPG MMO Vitals Card HUD, Moon Phase Lighting Cycle & Darkness, Living Ecology (dirt spread/grass collapse), 3x3 Crafting Table, Item Toss ('Q'/Ctrl+Q), Double-Jump Flight, Non-Freezing Death Simulation, 3D Held Items |
| v0.M7.0 | - | v0.M7 | Dual-channel BFS light engine (Sunlight/Blocklight), pitch black true night, distinct Menu & In-Game music with crossfading, distinct Survival vs Creative inventory catalogs |
| v0.M5.6 | - | v0.M5 | 10-stage PNG block break cracks, persistent save protection, BGM audio player & ambient loop synth, celestial sun/moon, Caveman AI skills |
| v0.M5.5 | - | v0.M5 | Item entities & pickup physics, 2x2 crafting grid, tool durability & mining multipliers (M5 closure) |
| v0.M5.4 | - | v0.M5 | Camera-space locked viewmodel hand, first-person torso removal, multi-directional 3D wall torches |
| v0.M5.3 | - | v0.M5 | Mob knockback impulse & recoil stun, combat aggro interest timer, wild passive mob spawning fix |
| v0.M5.2 | - | v0.M5 | Dynamic view bobbing, Source movement (air-strafing & bhop), $512\times 512$ world scale, pigmen & villages, loading screen, Steve model & F5 perspectives |
| v0.M5.1 | v0.7.0 | v0.M5 | 3D torches, particles, water physics, world types, custom name input, mob AI overhaul, F3 profiling overlay |
| v0.M5.0 | v0.6.0 | v0.M5 | Sprinting, dynamic FOV, neutral mobs (pigs/cows) |
| v0.M4.0 | v0.4.0 | v0.M4 | Named worlds, caves, ores, water, torches |
| v0.M3.1 | v0.5.0 | v0.M3 | Splashes, smooth lighting, tree archetypes, greedy meshing, frustum culling |
| v0.M3.0 | v0.2.0 | v0.M3 | Save/load, non-freezing inventory |
| v0.M2.1 | v0.1.2 | v0.M2 | Storage inventory modal |
| v0.M2.0 | v0.1.1 | v0.M2 | Legacy-console menus (M2 delivery), compact crosshair, bidirectional options |
| v0.M1.0 | v0.1.0 | v0.M1 | Versioning, ChunkPalette, RLE |

**v1.M1.0 is the current build** and enters **v1.M1 -- Environmental Sound Physics & Spatial Acoustics**.

## Current milestone

**v1.M1 -- Environmental Sound Physics & Spatial Acoustics (DELIVERED / COMPLETE)**:
- **Falling Leaves & Visuality Particle Physics (Delivered)**:
  - *Falling Leaves*: Dynamic canopy scanner detects leaf blocks exposed to air beneath trees, spawning drifting leaves with horizontal aerodynamic sway, wind oscillation, terminal velocity, and ground resting simulation.
  - *Visuality Combat Sparks & Critical Hits*: Striking mobs bursts gold and bright white high-velocity spark particles. Falling attacks automatically trigger heavy critical hit spark bursts ($24\text{ particles}$).
  - *Directional Blood Splatters*: Melee attacks on mobs and damage dealt to the player spray crimson blood droplets in the attack direction with gravity, velocity drag, and surface resting physics.
- **Multi-Biome Infinite Terrain Generation (Delivered)**: Continentalness and erosion-driven macro landscapes seamlessly generating:
  - *Lakes & Deep Waters*: Low depressions below sea level ($Y=26$) with sand/gravel beds and aquatic coastlines.
  - *Meandering Rivers*: Smooth sinusoidal 2D river carvers cutting continuous channels through hills and mountains, connecting bodies of water.
  - *Highlands & Plateaus*: Elevated rolling steppes ($Y=38\text{--}50$) with pine/spruce tree clusters and stone bluffs.
  - *Mountains & Jagged Summits*: Towering alpine peaks reaching $Y=50\text{--}74$ with exposed stone/cobblestone rocky summits and cliff faces.
  - *Plains & Dense Forests*: Gentle rolling meadows with dense tall grass and multi-tier oak and birch forest canopies.
  - *Smart Dry Spawn*: `Player::spawnAt` automatically finds dry shoreline land above water level to prevent spawning underwater.
- **Sound Physics Remastered & Presence Footsteps Integration**: Ray-traced sound occlusion through solid voxels via DDA, dynamic biquad lowpass filtering, cave/room reverberation feedback simulation, material-specific absorption coefficients (stone reflection vs wool/leaves damping), underwater fluid muffling, stride-cadence footstep sound triggers with surface substrate differentiation.
- **Improved Player Bobbing & View Dynamics**: Harmonic dual-axis camera translation, subtle authentic roll and pitch dip linked to footfall impacts, weighted hand/item stride inertia and sway, with an Options menu toggle ("View Bobbing" ON/OFF) to disable camera motion while maintaining gentle arm breathing.
- **FastNoiseLite Procedural Noise Engine & Multi-Octave Landforms (Delivered)**: Integrated [`FastNoiseLite`](file:///home/ais/VoxelMin/src/world/FastNoiseLite.hpp) for SIMD-friendly coherent noise evaluation:
  - *OpenSimplex2 Fractal FBm Continentalness & Erosion*: Macro landscape shaping with 4 octaves and smooth biome transitions.
  - *Ridged Multifractal Peaks*: Carving dramatic alpine mountain ranges reaching up to $Y=76$.
  - *Deep Ocean & Lake Depressions*: Low tectonic basins descending to $Y=10$ with aquatic floor beds.
  - *3D Perlin Noodle & Cheese Caverns*: Subterranean cave networks cutting through bedrock to surface terrain.
- **The Backrooms Level 0 Dimension ("The Yellow Hell") (Delivered)**:
  - *Infinite Cellular Maze Generation*: Procedural cellular partitioned corridors, office support pillars, dead ends, mono-yellow wallpaper, moist damp carpet, and acoustic drop ceilings with fluorescent light fixtures on a $4\times 4$ grid ([`BackroomsGenerator`](file:///home/ais/VoxelMin/src/world/BackroomsGenerator.hpp)).
  - *New Blocks & Items*: Added `BackroomsWallpaper`, `BackroomsCarpet`, `BackroomsCeiling`, `FluorescentLight` (emitted light 15), `GlitchBlock` (reality tear), `ExitDoor` (emergency fire exit), and `AlmondWater` consumable beverage (+45 hunger, +35 health) to [`BlockId`](file:///home/ais/VoxelMin/src/world/Block.hpp) and creative catalogs.
  - *Dimension Mechanics & Real-Time Noclip*: Stepping into or right-clicking `GlitchBlock` in the Overworld triggers instant Noclip into Level 0. Right-clicking `ExitDoor` in Level 0 teleports the player back to their exact saved Overworld location.
  - *Cinematic RPG Title Banners*: Dimension entry triggers gold-accented animated title cards (e.g. "LEVEL 0 - The Yellow Hell", "THE OVERWORLD - The Surface Realm") rendered via [`Renderer::drawTitleBanner`](file:///home/ais/VoxelMin/src/render/Renderer.cpp).
  - *Mono-Yellow Volumetric Atmosphere*: Dimension-specific sky and fog colors (damp yellow haze), tight fog rendering distance (14--32 blocks), and dual-dimension persistent chunk caching.
- **Missing Chunk Hole Fix & Base Terrain Flag (Delivered)**: Added `terrainGenerated` lifecycle flag on `Chunk` to ensure chunks populated with overhanging tree foliage or foundation blocks prior to full terrain generation are correctly marked and generated by `updateStreaming`, eliminating square void gaps.
- **Village Water & Ocean Avoidance Checker (Delivered)**: `VillageGenerator::locateVillages` performs a wide multi-ring perimeter footprint scan checking surface elevation ($\ge 28$), surface block type, and elevation variance ($\le 6$), strictly rejecting placement over lakes, rivers, or ocean waters.
- **Seamless View-Distance Spawn & World Loading (Delivered)**: World initialization and save loading pre-generate, calculate lighting, and GPU-mesh all chunks across the full visible view distance (`max(6, viewDistance + 1)`), eliminating missing chunk void gaps and initial stutter upon entering the world.
- **Persistent Explored Chunk Cache (Delivered)**: `World::m_savedChunkCache` stores RLE compressed voxel snapshots for all generated and modified chunks, guaranteeing unedited chunks are instantly reloaded from RAM/disk without regenerating noise when revisiting.
- **Fast Fluid Update Physics (Delivered)**: `scheduleFluidUpdate` with an active fluid queue and direct chunk voxel writes enables fast, natural waterfall cascades without frame drops.
- **4-Corner Per-Vertex Smooth Dynamic Lighting across chunk voxel meshing**: `computeVertexSmoothLight` samples neighbor voxels along face tangents and bitangents, blending sunlight and torchlight smoothly across vertices.
- FaceMask vertex lighting matching: Greedy mesher checks all 4 corner light levels before merging quads, preserving lighting gradients.
- Perceptual non-linear lighting curve: `pow(clamp(vLight, 0.0, 1.0), 1.25)` and `pow(clamp(vTorchLight, 0.0, 1.0), 1.25)` in chunk fragment shader.
- Trilinear entity ambient lighting: continuous world-space light interpolation for mobs and dropped item entities.
- Dynamic light-reactive breaking textures and particles: fracture overlays and dig particles darken in caves and at night.
- Recipe discovery unlocking system: recipes remain hidden until necessary crafting components are acquired.
- Interactive seed generation & string-to-number hashing: custom seed input on world creation.
- Third-person head articulation fix: proper camera-aligned pitch without inverted neck tilts.
- Distinct Workbench 3x3 Crafting UI vs Player Inventory Crafting UI.
- Authentic Legacy Console Edition crafting interface with 5 category tabs (`Structures`, `Tools & Weapons`, `Food & Essentials`, `Mechanisms`, `Decorations`), horizontal recipe selector, visual ingredient preview, yield slot, and 1-click recipe crafting.
- Interactive 3x3 Crafting Table workbench triggered via right-click raycasting on `BlockId::CraftingTable`.
- RPG MMO Vitals Card HUD anchored top-left with Steve portrait, Level badge, vertical XP bar, player name, red health bar, and Food, Oxygen, and Armor stats with `+20` sub-counters.
- 8-Phase Lunar Light Cycle ($0..7$) where Full Moon provides soft blue ambient illumination (`0.065f`) and New Moon produces pure pitch-black darkness (`0.002f`).
- Living world ecology: dirt exposed to sunlight naturally spreads into grass blocks over time, and unsupported tall grass collapses immediately when base ground is destroyed.
- Item drop physics: 5-minute item despawn, 1.6m magnetic draw, 0.95m pickup range, 'Q' key for single item toss, and Ctrl+'Q' for full stack toss with momentum impulse and pickup cooldown.
- Camera-locked first-person right arm viewmodel holding 3D mini-blocks or 2.5D tool sprites with walking bob and swing animations.
- Creative mode enhancements: automatic clean absorption of item pickups when inventory is full, and double-space / double-jump flight toggle.
- Non-freezing death screen: living world simulation (mobs, items, particles, day/night) continues uninterrupted behind Game Over UI.
- Multi-path world save deletion across all search paths.

**M7 -- Lighting & Visual Immersion (COMPLETE)**: Landed:
- Nibble-packed dual 4-bit Sunlight and 4-bit Blocklight storage per voxel in `Chunk`.
- BFS 3D flood-fill propagation queues with directional attenuation (15 sky down to caves, 14 torches radiating outward).
- Smooth 4-corner vertex ambient occlusion (AO values: 0.45, 0.65, 0.82, 1.0) with quad diagonal split flipping to prevent anisotropy artifacts.
- Multi-source shader blend combining sunlight, time-of-day sky factor, warm golden torch illumination (`vec3(0.18, 0.09, 0.02)`), and directional face shading.
- Pitch black true night and cave darkness (deep caves require torches, no torches automatically spawned in natural caves).
- Separate procedural background music for Main Menu (arpeggio piano theme) vs In-Game exploration (ambient pads) with smooth dynamic 1.0s crossfading.
- Distinct Survival Mode vs Creative Mode inventories.
- Fully integrated into entities (mobs, items, player body, first-person viewmodel arm).

**M5 -- Gameplay & Entities (COMPLETE)**: Landed:
10-stage PNG block break textures, persistent save directory & auto-migration, BGM audio player with miniaudio file loader & procedural ambient chord loop,
celestial sun & moon with 10-minute Day/Night cycle,
item entities & pickup physics, 2x2 crafting grid & tool durability/mining multipliers,
sprinting + dynamic FOV, Source-style movement (air-strafing and bunnyhopping),
dynamic view bobbing, camera-space locked first-person viewmodel arm,
Minecraft player model (Steve) with F5 perspective cycling (back & front),
directional 3D wall torches (West, East, North, South) with angled geometry and particle positioning,
neutral mobs with AI (pigs, cows, and village-defending pigmen),
mob knockback impulse & hitstun recoil, combat aggro interest timer, wild passive mob spawning (72+ pigs/cows across world),
world scale expansion ($512 \times 512$ blocks, 1024 chunks), procedural multi-template surface villages,
procedural menu panorama ($128 \times 128$) & zero-stutter pre-meshed world loading,
directional logs, dirt paths, batched mob rendering, 3D floor torches + particles, sub-block precision hitboxes,
wild tall grass, world deletion UI, water mechanics, world types, custom name input, GUI scaling, mob AI traversal, and the
F3 profiling overlay.

## Current metrics (latest shipped build, v0.M5.3)

Measured with the F3 overlay on the default `Default` world (seed 1337) at the
player spawn, 92 mobs alive (72 wild pigs/cows + 20 village pigmen). These figures come from the **shipped v0.M5.3
build** (the overlay is part of that release, not unreleased work). Reproduce
with:

```powershell
$env:VOXELMIN_CAPTURE="m.bmp"; $env:VOXELMIN_CAPTURE_STATE="play"
$env:VOXELMIN_CAPTURE_FRAMES="500"; $env:VOXELMIN_CAPTURE_VSYNC="0"
build\bin\VoxelMin.exe
```

| Metric | Value | Notes |
|--------|-------|-------|
| World size | 512 x 80 x 512 blocks | 32x32 chunks of 16x80x16 (1024 chunks) |
| Chunks | 1024 total; 500-700 frustum-culled | camera-distance sorted meshing queue |
| Voxel RAM | ~20.5 MiB | palette-compressed, startup log |
| Save size on disk | 343 KiB | VOXS v2 + RLE, `saves/world.dat` |
| Entities | 24 mobs (12 pigs + 12 cows) | no hard cap |
| Particles | 256 hard cap | 6-10 alive with no torches in view |
| Draw calls / frame | **88** | includes 1 entity batch, 1 particle batch, HUD + overlay 2D quads |
| Triangles / frame | **~87 200** | greedy-meshed chunk geometry + cross foliage |
| Frame time @720p, vsync off (avg / p99) | **0.33 ms / 0.52 ms** CPU frame loop | ~3000 fps |
| Frame time @720p, vsync on | 6.06 ms | display-limited: dev monitor is 165 Hz, not a 60 Hz cap |
| Frame time @1920x1061, vsync off | 0.35 ms | ~2850 fps |
| Automated tests / CI | none | the remaining M7 work (see `docs/ROADMAP.md`) |

**Caveat on frame time**: these are CPU-side frame-loop timings. OpenGL command
submission is asynchronous, so no GPU time is included. Real GPU cost is
unmeasured -- it would need `glFinish()` or `GL_TIME_ELAPSED` timer queries.
Draw calls, triangles and chunk counts are exact; frame time is a lower bound.
Measurements re-verified on v0.M5.2: Source movement and view bobbing additions
have negligible CPU overhead (<0.03 ms), while tall grass adds ~10k triangles
across the frustum.

## Open decisions (blocking M6)

M6 is the first milestone whose design is not already settled. These need an
answer before implementation starts, because each one changes the build system
or the save format and is expensive to reverse.

- **Audio backend**: OpenAL (MSYS2 `mingw-w64-x86_64-openal`), miniaudio
  (single-header, no extra dependency) or SDL_mixer. Not chosen. Affects the
  build script and asset licensing.
- **Threading model** for chunk generation and meshing: fixed worker pool with
  a task queue vs `std::async` per chunk vs staying single-threaded. The mesher
  currently rebuilds 8 chunks/frame on the main thread.
- **Chunk load/unload policy**: view-distance radius with hysteresis to avoid
  thrashing at the boundary, LRU eviction, or a strict per-frame budget. The
  Options screen already exposes "View Distance" as fog distance, so the two
  would need unifying.
- **Save format v3**: VOXS v2 assumes a single fixed 8x8 grid. v3 must handle an
  infinite world -- region files (32x32 chunks, Anvil-style) vs per-chunk files
  vs sharded directories. Decide migrate-or-reject up front; silently breaking
  every existing `saves/*.dat` is not acceptable.

Open, but not M6 blockers:

- **Mob persistence**: are mobs saved to disk, respawned deterministically from
  the seed, or ignored on load?
- **Water flow**: source cells are static today. Realistic finite-source flow is
  a much larger simulation than M6 needs.

## Done

### 2026-10-02 -- Milestone v1.M1: Falling Leaves, Visuality Combat Particles, Multi-Biome Infinite World & Dynamic Fluid Mechanics

- **Persistent Chunk Cache & Infinite World Preservation (Delivered)**:
  - Added `World::m_savedChunkCache` to store RLE-compressed voxel payloads for all explored/generated chunks across the world.
  - When walking back to any previously visited chunk, `generateSingleChunk` loads directly from the saved chunk cache via `decompressRle` (0 noise recalculation, 0 block resets, preserving player builds and terrain modifications).
  - On chunk unload during streaming, modified voxel data is automatically synced to `m_savedChunkCache`.
  - `WorldSave::saveGame` writes all explored chunks in the world to the `.dat` save file, and `WorldSave::loadGame` restores the entire persistent chunk cache.
- **Dynamic Water Cascade & Fluid Physics (Delivered)**:
  - Implemented active fluid simulation queue in `World::scheduleFluidUpdate` / `World::tickFluids`:
    - Water blocks placed in mid-air or exposed to air below immediately trigger downward waterfall cascades until reaching solid ground.
    - Water supported by solid ground spreads horizontally in 4 cardinal directions.
    - Digging/clearing blocks adjacent to or beneath water immediately wakes up neighbor fluid blocks to cascade into the opened space.
- **View Distance + 2 Chunk Streaming & FPS Optimization**:
  - Bound dynamic chunk generation strictly to `viewDistanceChunks + 2` radius (`maxGenRadius`), unloading any chunk beyond `viewDistanceChunks + 3`.
  - Balanced chunk streaming budget to 1 chunk per frame max to prevent frame time spikes.
  - Adjusted dirty mesh rebuild budget in `Application::rebuildDirtyMeshes` to 4 chunks per frame for high FPS.
  - Optimized stone ore distribution in `TerrainGenerator` from 4 separate 3D hash evaluations down to 1 single hash evaluation per voxel.
- **World Creation & Fluid Simulation Optimization (Freeze Fix)**:
  - Fixed infinite world freeze during world creation and runtime loop:
    - `TerrainGenerator::generateInitialSpawn` builds `m_loadedList` before lighting passes and village generation.
    - `World::getSunLight` returns 0 (instead of 15) for unloaded underground chunk lookups, stopping boundary light flooding.
    - Removed redundant full-volume `updateLightAround` calls from single-chunk meshing and streamlined localized light recalculation radius ($r=8$).
    - Added rate-limiting simulation timers for fluid spread (`m_fluidTickTimer = 0.15s`) and ecological grass spread (`m_ecologyTickTimer = 0.25s`).
    - Streamlined torch particle scanning to fast candidate checks in `ParticleSystem::update`, reducing per-frame block lookups from 14,000+ to 48.
- **Visuality Combat Sparks & Directional Blood Splatters**:
  - Implemented high-velocity golden impact sparks and critical hit particles (`ParticleType::Spark`) triggered when striking mobs (with extra bursts during falling critical strikes).
  - Implemented directional crimson blood splatters (`ParticleType::Blood`) with spray momentum, gravity acceleration, and ground contact resting upon mob attacks and player damage.
- **Multi-Biome Infinite Terrain Generation**:
  - Implemented continentalness and erosion noise generating Lakes & water basins ($Y=14\text{--}25$), sinusoidal meandering Rivers ($Y=21\text{--}26$), elevated Highlands & steppes ($Y=38\text{--}50$), alpine Mountain summits ($Y=50\text{--}74$), and multi-tier Oak/Birch/Pine forests.
  - Added smart dry spawn land search in `Player::spawnAt`.
- **Dynamic Water Flow Mechanics**:
  - Implemented cellular automaton fluid simulation in `World::tickFluids` causing water to cascade down sheer drops and spread horizontally across surfaces up to 4 blocks.
- **Spatial Audio & Footstep Acoustics**:
  - Implemented raytraced voxel sound occlusion, biquad lowpass filtering, and substrate-based footsteps based on Sound Physics Remastered and Presence Footsteps.

### 2026-10-02 -- Milestone M8: Console Crafting UI, RPG MMO Vitals Card HUD, Moon Phases & Living Ecology

- **Legacy Console Edition Crafting Interface & Interactive Workbench**:
  - Implemented authentic Legacy Console Edition UI (`drawConsoleInventory`) with 5 category tabs (`Structures`, `Tools & Weapons`, `Food & Essentials`, `Mechanisms`, `Decorations`), horizontal recipe selector, visual ingredient preview box, yield slots, and 1-click recipe crafting.
  - Added full recipe registry in `CraftingRecipes.hpp` covering all tools, weapons, armor, mechanisms, and decorative blocks.
  - Added interactive right-click on placed Crafting Tables opening 3x3 workbench recipes.
- **RPG MMO Vitals Card HUD**:
  - Implemented top-left HUD card (`drawRpgVitalsHud`) with player Steve portrait, Level badge ("1"), vertical XP bar, player name ("Deadzoke"), red health bar, and Food, Oxygen, and Armor stats with `+20` sub-counters. Automatically scales with GUI scale and window size.
- **8-Phase Moon Lighting Cycle**:
  - Implemented 8-phase lunar cycle ($0..7$) where Full Moon emits soft blue ambient moonlight (`0.065f`) and New Moon produces pure pitch-black night (`0.002f`).
- **Living World Ecology**:
  - Added `World::tickEcology`: dirt exposed to open sky/sunlight spreads into lush grass blocks over time, and unsupported tall grass collapses immediately when its supporting block is mined.
- **First-Person Viewmodel & 3D Held Items**:
  - Added dynamic held item rendering in camera-space right arm viewmodel: mini 3D blocks and angled 2.5D tool sprites with bobbing and swing animation.
- **Item Drop Physics & Creative Double-Jump Flight**:
  - 5-minute despawn, 1.6m magnetic draw range, 0.95m pickup range, 'Q' key for single item toss, Ctrl+'Q' for full stack toss with trajectory momentum and pickup cooldown.
  - Creative mode automatically absorbs full inventory pickups cleanly and uses double-space / double-jump within 0.28s to toggle flight.
- **Non-Freezing Death Simulation & Multi-Path World Deletion**:
  - Game Over state continues running entity, item, particle, and day/night simulation.
  - World deletion cleans up `.dat` save files across all directory search paths.

### 2026-10-02 -- Sky camera lock fix, dirt path drops dirt, shovel tilling

- **Sky was locked to a frozen camera**: `Renderer::drawSky` was called with the menu camera during play while the world was drawn with the player camera, so the sun/stars/sky never followed the player's view. `renderScene` now selects the player camera for the sky in `Playing`/`Inventory`/`GameOver` (menu states keep the orbiting menu camera). This was the root cause of the original "sky is locked on my pov" complaint.
- **Sky dome gradient verified world-locked**: the dome is world-aligned and follows the camera position (`uCamPos`), so the zenith/horizon gradient rotates with the player's view. Captures confirm: looking straight up shows the sun over deep-blue zenith; the horizon band matches the fog colour.
- **Dirt Path drops Dirt**: `getDropForBlock` mapped `DirtPath` to itself; it now returns `Dirt` like `Grass`.
- **Shovel tilling**: right-clicking a `Grass` or `Dirt` block with any shovel converts it to `Dirt Path` (with dig sound, debris particles and shovel durability loss in survival). Tilling takes precedence over block placement.

### 2026-10-02 -- Death keeps world simulating, world-delete resurrection fix, creative-only flight, view-direction sky gradient

- **Death no longer freezes the world**: `GameOver` state now updates `EntityManager` every frame, so mobs wander, items bob and particles drift behind the death screen. `Player::takeDamage` early-returns while `m_isDead`, so mobs cannot deal further damage to the corpse.
- **World deletion resurrection fixed**: `WorldSave::listSavedWorlds()` auto-migrated legacy save copies from stale build folders back into the persistent `saves/` directory *after* a deletion, bringing deleted worlds back on the next scan. Migration now removes the legacy copy after a successful copy, and the stale `build/bin/saves/` directory was deleted. Verified with a standalone test: delete -> re-scan no longer resurrects.
- **Flight is creative-only**: `F` toggles flying only while creative mode is on; switching back to survival force-disables flight; loaded saves never restore a saved flying state (creative mode is not persisted, so restoring flight would be inconsistent).
- **Sky gradient dome**: the sky was a flat clear colour, identical in every view direction. Added a camera-centred UV-sphere dome (`skydome.vert`/`skydome.frag`) drawn first in `Renderer::DrawSky` with a zenith/horizon/ground gradient derived from the time-of-day sky colour. Horizon matches the fog colour so distant terrain blends seamlessly; the sky now changes as the player looks around.

### 2026-10-01 -- Milestone M6 Audio Engine: Miniaudio Integration & Core Procedural SFX

- **Miniaudio Audio Engine Architecture**:
  - Vendored single-header `miniaudio.h` (`src/audio/miniaudio.h`, `src/audio/miniaudio_impl.cpp`) compiling warning-free under `-Wall -Wextra -Wpedantic`.
  - Implemented high-performance low-latency mixer in `AudioEngine` (`src/audio/AudioEngine.hpp`, `src/audio/AudioEngine.cpp`) operating at 44.1 kHz stereo with 32 active polyphonic voices, soft limiting, and graceful silent fallback on headless systems.
  - Implemented spatial 3D audio via `play3D()` calculating distance quadratic attenuation ($1 / (1 + 0.14 d^2)$) and orientation-aware stereo panning ($L / R$).
  - Added Master Volume, Sound Effects, and Ambient Wind sliders to the in-game and title Options menu with live tuning.

- **Procedural Sound Effects (Zero External Asset Overhead)**:
  - Procedural sound synthesis generates rich authentic waveforms at startup without requiring large external audio asset packs:
    - **UI Click**: Crisp damped tactile mechanical switch sound for buttons and slot interactions.
    - **Item Pickup**: Pleasant upward frequency chime/pop ($620 \rightarrow 1260\text{ Hz}$).
    - **Footsteps**: Material-specific audio (Grass/Dirt rustle, Stone hard clack, Wood hollow knock) triggered via movement distance tracking.
    - **Block Breaking**: Material-specific dig audio (Grass crunch, Stone rock fracture, Wood splinter snap).
    - **Block Placement**: Solid resonant thud impact clack.
    - **Mob Hurt**: Fleshy punch thud with gentle soft clipping.
    - **Tool Break**: High-frequency metallic snap and fracture chime.
    - **Ambient Wind**: Continuous seamless 4-second pink-noise breeze with soft sine swell mixed in-game.

### 2026-10-01 -- VoxelMin v0.M5.5: Item Entities, Physics Drops, 2x2 Crafting Grid & Tool Durability (Milestone M5 Complete)

- **Item Entities & Pickup Physics**:
  - Implemented `ItemEntity` (`src/entity/ItemEntity.hpp`, `src/entity/ItemEntity.cpp`) with full 3D voxel AABB collision physics, gravity ($22.0\text{ m/s}^2$), ground friction, and terminal velocity.
  - Added magnetic attraction towards the player within a $2.4\text{ m}$ radius once the initial spawn pickup cooldown ($0.6\text{ s}$) expires, smoothly vacuuming items into inventory upon touching ($0.9\text{ m}$ radius).
  - Floating items render as bobbing, rotating mini 3D voxel cubes sampling skylight from the world, fully batched into dynamic entity vertex buffers with zero additional draw calls.
  - Integrated block breaking drops via `getDropForBlock(BlockId)`: stone drops cobblestone, coal ore drops coal, diamond ore drops diamond, grass drops dirt, tree leaves have a chance to drop sticks.
  - Integrated player inventory insertion via `addItem(BlockId, count)` prioritizing hotbar then inventory.

- **2x2 Crafting Grid & Authentic Crafting Recipes**:
  - Integrated 2x2 crafting grid into the inventory modal with 4 crafting input slots, an authentic ASCII arrow (`=>`), and an interactive craft result slot.
  - Supported crafting recipes:
    - 1 Wood $\rightarrow$ 4 Planks
    - 2 Planks (vertical) $\rightarrow$ 4 Sticks
    - 4 Planks (2x2) $\rightarrow$ 1 Crafting Table
    - 1 Coal + 1 Stick $\rightarrow$ 4 Torches
    - Wooden Pickaxe, Axe, Shovel, and Sword
    - Stone Pickaxe, Axe, Shovel, and Sword
  - Full stack manipulation: left-click to craft/swap, right-click to place single items, and auto-crafting result preview. Leftover items are automatically returned to inventory or dropped on inventory close.

- **Tool Durability & Combat / Mining Multipliers**:
  - Added `ItemSlot` structure holding `BlockId`, stack `count` (up to 64), and current `durability`.
  - Added tiered durability: Wood tools (59 uses), Stone tools (131 uses), Iron tools (250 uses), Diamond tools (1561 uses).
  - Implemented Minecraft-style colored durability bars (green $\rightarrow$ yellow $\rightarrow$ red) rendered beneath tools in both the HUD hotbar and the inventory modal.
  - Tools consume 1 point of durability per block broken or mob hit, breaking cleanly when durability reaches zero.
  - Integrated tool-specific attack damages (Stone Sword 6, Wood Sword 5, Axes 4, Pickaxes 3, Shovels 2) and mining multipliers for faster harvesting.
  - Released **v0.M5.5**, officially completing Milestone M5!

### 2026-09-29 -- VoxelMin v0.M5.4: Camera-Space Locked First-Person Viewmodel & Directional 3D Wall Torches

- **First-Person Viewmodel Screen Anchoring & Torso Removal**:
  - In first-person perspective, removed the player body/torso/leg rendering (`Player::appendGeometry` now returns immediately in `Perspective::FirstPerson`), preventing awkward torso mesh clipping into the camera when looking straight down.
  - Implemented dedicated camera-space viewmodel rendering in [`Renderer::drawFirstPersonArm`](file:///c:/Users/Ais-PC/VoxelMin/src/render/Renderer.cpp) with an identity view matrix and depth clearing.
  - Generates the arm mesh directly in camera view coordinates in [`Player::appendFirstPersonArm`](file:///c:/Users/Ais-PC/VoxelMin/src/player/Player.cpp), locking the hand cleanly to the bottom-right corner across all yaw and pitch angles (including looking straight up at the sky $+85^\circ$ or straight down at the ground $-85^\circ$) while preserving walk bobbing and punch/swing animations.

- **Multi-Directional 3D Wall Torches**:
  - Added directional wall torch block IDs: `BlockId::TorchWallEast`, `BlockId::TorchWallWest`, `BlockId::TorchWallSouth`, and `BlockId::TorchWallNorth` in [`src/world/Block.hpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/Block.hpp).
  - Defined custom precision `BlockBounds` for all 4 wall orientations matching Minecraft wall torch dimensions ($[0.0, 0.2, 0.35]$ to $[0.4, 0.8, 0.65]$ etc.).
  - Implemented 3D slanted geometry meshing in [`ChunkMesher::emitTorch3D`](file:///c:/Users/Ais-PC/VoxelMin/src/world/ChunkMesher.cpp), rendering a $2 \times 2$ pixel thick post slanted outwards from the attached wall face with a glowing $3 \times 3$ pyre head.
  - Updated [`ParticleSystem::spawnTorchFlames`](file:///c:/Users/Ais-PC/VoxelMin/src/render/ParticleSystem.cpp) to spawn animated flame and smoke particles precisely at the slanted pyre position for each wall torch orientation.
  - Integrated smart wall placement in [`Application::updateInteraction`](file:///c:/Users/Ais-PC/VoxelMin/src/core/Application.cpp): right-clicking any vertical block face with a torch automatically mounts the torch slanted onto that wall face, while top face clicks place standard upright floor torches.

### 2026-09-29 -- VoxelMin v0.M5.3: Mob Knockback Recoil, Hitstun Stun, Combat Interest Timer & Wild Mob Spawning Fix

- **Mob Knockback Impulse & Hitstun Recoil**:
  - In [`Mob::takeDamage`](file:///c:/Users/Ais-PC/VoxelMin/src/entity/Mob.cpp), mobs receive an authentic physics impulse launching away from the damage source (`knockDir * 7.5f` horizontally and `+4.8f` vertical pop).
  - Added `m_knockbackTimer = 0.35f;` during which `Mob::updatePhysics` preserves and decays horizontal knockback velocity with friction (`7.0f` on ground, `2.0f` in air) rather than instantly overriding velocity with locomotion speed.
  - While in hurt stun (`m_hurtTimer > 0.0f`), mobs flinch in place (`m_moveSpeed = 0.0f;`) and orient towards the attacker rather than immediately charging forward.

- **Combat Interest / Aggro Timeout**:
  - Implemented an aggro timer (`m_aggroTimer = 12.0f`) for retaliating Pigman Villagers in [`Mob.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/entity/Mob.cpp).
  - Pigmen pursue attackers during the active interest window, refreshing the timer whenever they land a melee strike.
  - If the player flees beyond pursuit radius (`> 22.0m`), or the pigman wanders outside village defense boundaries (`> 34.0m`), or the interest timer expires (`m_aggroTimer <= 0.0f`), the pigman cleanly loses interest, calms down, and navigates back to its village plaza.

- **Wild Passive Mob Spawning Fix (Pigs & Cows Across the World)**:
  - Fixed an issue in [`EntityManager::spawnDefaults`](file:///c:/Users/Ais-PC/VoxelMin/src/entity/EntityManager.cpp) where tree foliage, logs, and wild tall grass caused `surfaceHeight` checks to fail surface block validation.
  - Implemented downward solid ground probing past leaves and tall grass to find true solid grass/dirt blocks with open headroom.
  - Scaled wild mob distribution to spawn 72 wild passive animals (36 pigs + 36 cows) across open pastures and hills in $512 \times 512$ worlds (92 total mobs including 20 village pigmen).

### 2026-09-29 -- VoxelMin v0.M5.2: Fast Procedural Menu Panorama, Zero-Stutter Chunk Pre-Meshing, Minecraft Player Model & F5 Perspectives

- **Instant Main Menu Launch & Dynamic Procedural Panorama ($128 \times 128$)**:
  - Refactored [`World`](file:///c:/Users/Ais-PC/VoxelMin/src/world/World.hpp) and [`TerrainGenerator`](file:///c:/Users/Ais-PC/VoxelMin/src/world/TerrainGenerator.hpp) to support dynamic world dimensions via `World::init(chunksX, chunksZ, seed)`.
  - Main menu initializes a compact, scenic $8 \times 8$ chunk panorama ($128 \times 128$ blocks, 64 chunks) instead of the full $512 \times 512$ world, slashing startup voxel RAM from ~20.5 MiB to ~1.2 MiB and initial meshing time from ~4 seconds to < 20 ms.
  - Adapted [`VillageGenerator`](file:///c:/Users/Ais-PC/VoxelMin/src/world/VillageGenerator.cpp) dynamically: automatically scales from 1 central village for menu worlds (< 180 blocks) to 4 regional settlements for $512 \times 512$ gameplay worlds.

- **Zero-Stutter World Startup & Live Chunk Pre-Meshing**:
  - Integrated full chunk pre-meshing into the animated loading screen for both new world creation and world loading in [`Application.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/core/Application.cpp) (`startNewWorld`, `loadWorld`).
  - Progress bar advances smoothly from 85% $\to$ 99% during pre-meshing (`Building chunk meshes... (x/1024)`), rendering in real-time.
  - Upon entering gameplay, 100% of visible chunk geometry is already GPU-uploaded, eliminating generation stutter and visual pop-in completely.

- **Minecraft-Style Biped Anatomy & Player Character Model (Steve)**:
  - Added procedural player textures in [`Block.hpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/Block.hpp) and [`Texture.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/render/Texture.cpp) (`PlayerFace`, `PlayerHead`, `PlayerTorso`, `PlayerArm`, `PlayerPants`, `PlayerShoe`), recreating authentic Steve styling (brown hair, indigo eyes, goatee/mouth, cyan t-shirt, denim blue trousers, and gray shoe soles).
  - Modeled player body geometry in [`Player::appendGeometry`](file:///c:/Users/Ais-PC/VoxelMin/src/player/Player.cpp) using authentic Minecraft proportions:
    - Head: $0.48 \times 0.48 \times 0.48\text{m}$ (neck at $Y=1.44\text{m}$, articulating with look pitch & yaw).
    - Torso: $0.48 \times 0.72 \times 0.24\text{m}$ ($Y \in [0.72, 1.44]\text{m}$).
    - Arms: $0.24 \times 0.72 \times 0.24\text{m}$ (shoulder pivots at $Y=1.44\text{m}$, $X=\pm 0.36\text{m}$).
    - Legs: $0.24 \times 0.72 \times 0.24\text{m}$ (hip pivots at $Y=0.72\text{m}$, $X=\pm 0.12\text{m}$).
  - Synchronized Pigman Villager proportions and limb placements in [`Mob.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/entity/Mob.cpp) to match standard humanoid biped anatomy.

- **First-Person Hand Viewmodel & Downward Look Feet Visibility**:
  - Implemented camera-anchored first-person right arm viewmodel via [`Player::appendFirstPersonArm`](file:///c:/Users/Ais-PC/VoxelMin/src/player/Player.cpp) and [`Renderer::drawFirstPersonArm`](file:///c:/Users/Ais-PC/VoxelMin/src/render/Renderer.cpp).
  - Viewmodel responds dynamically to walk/sprint bobbing and animates smooth swinging on left/right click block breaking and placing.
  - Added depth buffer clearing prior to viewmodel pass (`glClear(GL_DEPTH_BUFFER_BIT)`), preventing hands from clipping into nearby block faces.
  - Enabled downward feet visibility: when tilting pitch downward (`pitch < -10.0^\circ`), player's body and animated swinging legs render in world-space so players can look down and see their feet walking across terrain.

- **F5 Third-Person Perspective Cycling (Front & Back) with Anti-Clipping**:
  - Implemented `Perspective` enum and `Player::cyclePerspective()` toggled with `F5` (`FirstPerson` $\to$ `ThirdPersonBack` $\to$ `ThirdPersonFront` $\to$ `FirstPerson`).
  - Added camera collision raycasting against solid world geometry along the camera offset vector, smoothly preventing third-person cameras from clipping inside terrain or building walls.
  - Aligned model yaw and head pitch transformations so player model faces look direction seamlessly in both third-person perspectives.

### 2026-09-29 -- VoxelMin v0.M5.2: Terraria-Style World Generation Loading Screen, Dirt Path Block, Directional Logs & Red Damage Flash

- **Live Progressive World Generation Loading Screen**:
  - Implemented [`Application::renderLoadingScreen`](file:///c:/Users/Ais-PC/VoxelMin/src/core/Application.cpp) with an animated obsidian/charcoal card, bold gold title header, dynamic percentage counter, beveled gradient progress bar, real-time status messages, and gameplay tips.
  - Piped [`ProgressCallback`](file:///c:/Users/Ais-PC/VoxelMin/src/world/TerrainGenerator.hpp) through [`TerrainGenerator::generate`](file:///c:/Users/Ais-PC/VoxelMin/src/world/TerrainGenerator.cpp) and [`World::generate`](file:///c:/Users/Ais-PC/VoxelMin/src/world/World.cpp) with progressive phase milestones (terrain carving, cavern excavation, forest planting, village generation, entity spawning, snapshot saving), swapping buffers in real-time to eliminate window freeze.

- **Dirt Path Block (`BlockId::DirtPath`)**:
  - Added `BlockId::DirtPath`, `TextureTile::DirtPathTop`, and `TextureTile::DirtPathSide` in [`Block.hpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/Block.hpp).
  - Procedurally generated compacted earthen top and layered soil side textures in [`Texture.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/render/Texture.cpp) (`makeDirtPathTop`, `makeDirtPathSide`).
  - Added `DirtPath` to creative inventory and hotbar defaults.

- **Directional Log Placement (`WoodX`, `WoodZ`, `Wood`)**:
  - Added horizontal directional log blocks `BlockId::WoodX` (East-West) and `BlockId::WoodZ` (North-South) alongside vertical `BlockId::Wood` (Y-axis).
  - Configured face-normal texture mapping in [`ChunkMesher.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/ChunkMesher.cpp) (`getFaceTile`) so log bark and tree rings align properly on horizontal and vertical orientations.
  - In [`Application::updateInteraction`](file:///c:/Users/Ais-PC/VoxelMin/src/core/Application.cpp), right-click placement automatically detects targeted face normals (`normal.x != 0` -> `WoodX`, `normal.z != 0` -> `WoodZ`, `normal.y != 0` -> `Wood`).

- **Vibrant Red Damage Indicator Flash**:
  - Updated [`chunk.frag`](file:///c:/Users/Ais-PC/VoxelMin/assets/shaders/chunk.frag) and [`Mob.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/entity/Mob.cpp): when mobs take damage (`m_hurtTimer > 0.0f`), negative vertex AO signals the fragment shader to blend an authentic vibrant red damage overlay (`vec3(1.0, 0.18, 0.18)` at 72% intensity), replacing dark/black fading.

- **Enhanced Village Architecture & Pathways**:
  - Upgraded [`VillageGenerator.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/VillageGenerator.cpp):
    - Pathways laid with `DirtPath` and cobblestone stepping stone accents.
    - Cottages, town halls, wells, and watchtowers use directional logs (`WoodX`, `WoodZ`) for horizontal lintels and perimeter wall plates.
    - Houses built with wood plank floors, cobblestone wainscoting, overhanging eave roofs, interior workbench tables, and lit wall torches.
    - Foundations automatically clear foliage and tall grass across the footprint and underpin down to solid terrain.

### 2026-09-29 -- VoxelMin v0.M5.2: World Scale ($512 \times 512$), Neutral Pigman Villagers & Procedural Surface Villages

- **Expanded World Scale to $512 \times 512$ Blocks ($32 \times 32$ Chunks)**:
  - Scaled world dimensions in [`Config.hpp`](file:///c:/Users/Ais-PC/VoxelMin/src/core/Config.hpp) (`WORLD_CHUNKS_X = 32`, `WORLD_CHUNKS_Z = 32`), increasing playable world area sixteen-fold from 64 chunks to 1,024 chunks.
  - Chunk allocation, meshing, and saving/loading dynamically adapt to 1,024 chunks (~20.5 MiB total voxel RAM).
  - Extended atmospheric fog range (`FOG_START = 80.0f`, `FOG_END = 160.0f`) to reveal expansive horizons.
  - **Camera-Distance Sorted Mesher Queue**: In [`Application.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/core/Application.cpp) (`rebuildDirtyMeshes`), dirty chunks are sorted by 3D distance to the player camera before rebuilding, prioritizing immediately visible chunks and eliminating visual chunk pop-in.

- **Neutral Pigman Villager Mob & Village AI**:
  - **Species & Model**: Added `MobType::PigmanVillager` in [`MobAi.hpp`](file:///c:/Users/Ais-PC/VoxelMin/src/entity/MobAi.hpp) and procedural textures (`PigmanFace`, `PigmanSkin`, `PigmanTorso`, `PigmanHoof`) in [`Texture.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/render/Texture.cpp). Built with authentic bipedal geometry (1.95m height, 20 HP, protruding snout, articulated swinging arms that raise in combat stance, and dark hoof boots).
  - **State Machine & Neutral Temperament**:
    - **Peaceful Living**: Wanders calmly around its home village, idles near structures, and looks curiously at visiting players (`AiLookAt`, `AiWander`).
    - **Retaliation & Enrage**: Attacking a Pigman Villager triggers `AiRetaliate`, switching it to `Hostile` mode (sprints at 5.5 blocks/s, raises arms, and deals melee strikes).
    - **Group Defense**: Attacked pigmen broadcast distress to fellow villagers via `EntityManager::alertNearbyPigmen(pos, radius)`, rallying nearby pigmen to defend the village.
    - **Home Leash & Forgiveness**: If the player flees beyond the village leash radius (`kLeashDistance = 38.0m`), or the mob chases too far from its home anchor, the Pigman Villager calms down (`m_aggroTarget = -1`), transitions to `ReturnToVillage`, and navigates back to its home plaza.

- **Procedural Multi-Template Surface Village Generation**:
  - Created [`VillageGenerator.hpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/VillageGenerator.hpp) and [`VillageGenerator.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/VillageGenerator.cpp) integrated with [`TerrainGenerator.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/TerrainGenerator.cpp).
  - **Surface Conforming & Solid Foundations**: Implemented `prepareFoundation` to probe the true highest solid ground for every building, laying solid cobblestone underpin foundations down to bedrock level. Ensures villages are firmly built on the surface and never spawn floating in the sky or buried underground.
  - **Multi-Template Layout Diversity**: 4 distinct procedural archetypes chosen deterministically per village:
    1. *Crossroad Settlement*: 4-way intersecting cobblestone streets connecting cottages, farmlands, and a central well.
    2. *Valley High Street*: Linear main road lined with dense cottages, lamp posts, and a lookout tower.
    3. *Ring Commons / Market Square*: Central plaza well surrounded by concentric residential houses and irrigated crop gardens.
    4. *Fortified Enclave*: Clustered defense settlement with a tall stone watchtower, timber hall, and fortified perimeter.
  - **Architectural Detailing**: Cobblestone wells with water, cottages with log pillars, wood plank walls, glass windows, open doorways, gable roofs, irrigated farmlands with water canals and crops, and wooden torch posts providing night illumination.
  - **Automatic Population**: Naturally populates each generated village with 4–6 resident Pigman Villagers who tether to the central well as their home anchor.

### 2026-09-29 -- VoxelMin v0.M5.2: Smooth Minecraft View Bobbing & Responsive Movement (Air-Strafe & Bhop)

- **Version Bump**: Bumped version to `v0.M5.2` in [`Config.hpp`](file:///c:/Users/Ais-PC/VoxelMin/src/core/Config.hpp).
- **Smooth Minecraft-Style View Bobbing (Dizziness-Free & Level Horizon)**:
  - Refined view bobbing in [`Player.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/player/Player.cpp) and [`Camera.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/render/Camera.cpp) to follow Minecraft's smooth translation model:
    - **Stable Horizon**: Removed camera pitch/roll rocking and strafe-tilting (`m_roll = 0.0f; bobPitch = 0.0f;`). The camera orientation remains completely level, eliminating simulator sickness and motion blur.
    - **Smooth C-infinity Vertical Dip**: Replaced non-differentiable `|sin(t)|` bounce with a continuous smooth cosine wave (`-(cos(t) * 0.5 + 0.5) * 0.026m`), removing the jarring micro-shudder at the bottom of each footstep.
    - **Horizontal Sway**: Natural left-right weight sway (`sin(0.5 * t) * 0.022m`) smoothly oscillating across footstep cycles.
    - **Air & Water Damping**: Decays smoothly (`dt * 6.0`) when airborne or swimming, seamlessly returning to center eye position.
- **Responsive Voxel Movement & Air-Strafing**:
  - **Snappy Ground Control**: Direct, responsive ground acceleration (`GROUND_ACCEL_RATE = 18.0f`) and crisp deceleration (`GROUND_DECEL_RATE = 20.0f`), providing immediate WASD steering in all directions (forward, backward, strafe left/right, and diagonals).
  - **Full Mid-Air Steering & Sideways Jumping**: Airborne movement allows full directional steering (`AIR_ACCEL_RATE = 8.5f`), enabling players to easily strafe left/right in mid-air, jump around corners, and navigate parkour.
  - **Bunnyhopping & Jump Queueing**: Jump requests are buffered (`JUMP_QUEUE_DURATION = 0.15s` or held space bar) and launch immediately on ground contact, preserving directional momentum.

### 2026-09-29 -- VoxelMin v0.M5.2: Wild Tall Grass Render Stability (Anti-Flicker) & World Deletion UI

- **Fixed Grass Foliage Flickering, Z-Fighting & Dark Mipmap Edge Artifacts**:
  - **Eliminated Coplanar Overlapping Quads**: In [`ChunkMesher.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/ChunkMesher.cpp), `emitCrossModel` was emitting both front and reverse quads on the exact same diagonal plane. Because `GL_CULL_FACE` is disabled in Pass 2 (`Renderer::drawWorld`), this produced duplicate coplanar triangles fighting for depth. Simplified to 2 diagonal quads total, cutting vertex count in half and eliminating z-fighting.
  - **Alpha Cutout Threshold**: Updated `chunk.frag` discard test from `< 0.1` to `< 0.5` (`if (texel.a < 0.5) discard;`), eliminating semi-transparent depth occlusions and sub-pixel edge shimmer on foliage cutouts.
  - **Color Bleed Prevention in Mipmaps**: In [`Texture.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/render/Texture.cpp) (`makeTallGrass`), filled transparent background pixels with matching grass green RGB instead of black `{0, 0, 0}`, preventing mipmaps from downsampling into dark border fringes at a distance.

- **World Deletion Management & Confirmation UI**:
  - Added `WorldSave::deleteWorld(path)` in [`WorldSave.hpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/WorldSave.hpp) and [`WorldSave.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/WorldSave.cpp) using `std::filesystem::remove`.
  - Added `GameState::DeleteWorld` and `GameState::ConfirmDeleteWorld` states in [`Application.hpp`](file:///c:/Users/Ais-PC/VoxelMin/src/core/Application.hpp).
  - Added "Delete a World" menu in `Select World` listing all saved worlds via `WorldSave::listSavedWorlds()`, plus a confirmation dialog ("Are you sure you want to delete '<name>'?") to prevent accidental deletions.
  - Added `Delete` key keyboard shortcut (`GLFW_KEY_DELETE`) in `Select World` to instantly trigger delete confirmation for the currently selected saved world.

### 2026-09-29 -- VoxelMin v0.M5.2: Torch Hitbox Refinement & Wild Tall Grass Block

- **Sub-Block Hitbox & Precision Raycast for Torches and Plants**:
  - Previously, torches used the default full 1x1x1 voxel bounding box, causing rays and selection outlines to highlight the entire air block around the torch.
  - Implemented `BlockBounds` and `blockBounds(BlockId)` in [`Block.hpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/Block.hpp), specifying tight bounding boxes:
    - `Torch`: `[0.375, 0.0, 0.375]` to `[0.625, 0.65, 0.625]` (0.25 x 0.65 x 0.25 centered stick + flame).
    - `TallGrass`: `[0.15, 0.0, 0.15]` to `[0.85, 0.80, 0.85]` (0.70 x 0.80 x 0.70 foliage bounding volume).
  - Enhanced voxel DDA raycaster in [`Raycast.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/Raycast.cpp) with sub-block AABB intersection tests (`rayAABB`), enabling players to click/aim through the open air space around torches to hit geometry behind them.
  - Updated selection outline in [`Renderer.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/render/Renderer.cpp) to scale and offset the wireframe box to fit the exact `BlockBounds` of the targeted block.

- **Wild Tall Grass Foliage Block**:
  - Added `BlockId::TallGrass` and `TextureTile::TallGrass` in [`Block.hpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/Block.hpp).
  - Added cross-model diagonal quad meshing (`emitCrossModel`) in [`ChunkMesher.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/ChunkMesher.cpp) rendered in the transparent/cutout mesh pass with double-sided winding.
  - Added procedural 16x16 pixel-art generator in [`Texture.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/render/Texture.cpp) (`makeTallGrass`) generating organic grass blade clusters with alpha transparency cutouts.
  - Integrated natural surface foliage clustering into [`TerrainGenerator.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/TerrainGenerator.cpp) using 2D noise patches and seeded hashing across grassy surface terrain.
  - Added `BlockId::TallGrass` to player inventory in [`Application.hpp`](file:///c:/Users/Ais-PC/VoxelMin/src/core/Application.hpp).

### 2026-09-29 -- VoxelMin v0.M5.2: Natural mob jumping physics over ledges (removed step-up teleportation)

- **Replaced 1-block step-up teleportation with natural physics jumping**:
  - Previously, `tryStepUp` instantly snapped `m_position.y += 1.05f` in a single frame when a mob hit an elevation, looking like an instantaneous teleport.
  - Disabled `AiStepUp` for default mob profiles (`kPigGoals` and `kCowGoals`) in [`MobAi.hpp`](file:///c:/Users/Ais-PC/VoxelMin/src/entity/MobAi.hpp). *(Note: This supersedes the instant step-up approach added in v0.M5.1; AiStepUp is retained in the MobAi goal mask definition for optional/experimental use but disabled for default species.)*
  - Tuned `kJumpSpeed = 7.5f` (with gravity 24.0 m/s^2) so mobs naturally launch into a smooth parabolic jump arc clearing ~1.17m while maintaining forward momentum across the step. Reduced `kJumpCooldown` to 0.35s.
- **Fixed mob leg clipping and separation** (the "mobs foot is clipping from the body" bug):
  - **Pig Model**: Inset leg attachment `X` from `+/-0.20f` to `+/-0.15f` and `Z` from `+/-0.28f` to `+/-0.22f` (with leg dimensions 0.16w x 0.32h x 0.16l), maintaining a safe margin inside the 0.60w x 0.85l torso.
  - **Cow Model**: Inset leg attachment `X` from `+/-0.24f` to `+/-0.20f` and `Z` from `+/-0.36f` to `+/-0.28f` (with leg dimensions 0.18w x 0.58h x 0.18l), preventing the outer side faces from z-fighting and slicing through the 0.68w x 1.05l torso side panels. Adjusted udder dimensions to 0.20w x 0.14h x 0.24l to cleanly fit between the inner leg bounds.
  - **Leg swing amplitude**: Clamped maximum quadruped leg swing amplitude from 0.55 rad (~31.5 deg) to 0.32 rad (~18.3 deg), keeping back feet from swinging past the mob's rear geometry and front feet from punching out through the chest.
- **Verification**: Clean build with zero warnings (`-Wall -Wextra -Wpedantic`) and captured high-resolution verification screenshots from multiple camera angles.

### 2026-09-28 -- VoxelMin v0.M5.1: F3 profiling overlay + measured performance baseline

- **F3 debug overlay** (toggles in every game state, menus included):
  version/framebuffer, FPS + frame time, draw calls, triangles, chunk
  visible/drawn/culled, mob and particle counts, and player position.
- **`FrameStats`** ([`FrameStats.hpp`](file:///C:/Users/Ais-PC/VoxelMin/src/render/FrameStats.hpp)
  / `FrameStats.cpp`): per-frame counters reset in `Renderer::beginFrame()`,
  folded into a fixed 90-frame rolling average by `Application`. The first 20
  frames are skipped so world generation and first-frame shader compiles do not
  poison the average.
- **Instrumented every draw path**: chunk opaque and transparent meshes (using
  the new `Mesh::triangleCount()`), the entity batch, the particle batch, the
  selection outline, and all 2D quads (single `uploadUI` funnel) plus text and
  textured sprites.
- **Fixed a pre-existing culling inefficiency**: `drawWorld` frustum-tested
  every chunk AABB twice, once per mesh pass. Visibility is now computed once
  into a reusable buffer and shared by both passes -- less work, and it makes
  the "chunks culled" figure correct (it was double-counting).
- **Overlay excludes itself**: `drawDebugOverlay` snapshots and restores the
  counters, so the draw-call count it reports does not include its own quads.
- **Headless measurement**: captures now log a `Stats:` line alongside the BMP,
  and `VOXELMIN_CAPTURE_DEBUG=1` / `VOXELMIN_CAPTURE_VSYNC=0` force the overlay
  on and drop the vsync cap, so the metrics table is reproducible from a script.
- Filled the three `not instrumented` rows of `Current metrics` with measured
  values: **86 draw calls**, **~77 100 triangles**, **0.30 ms** CPU frame time
  at 720p. Documented the caveat that frame time is CPU-side only (no GPU
  timing) and that vsync-on is display-limited at 165 Hz, not 60.

### 2026-09-28 -- VoxelMin v0.M5.1: Mob AI overhaul: goal mask, terrain traversal, unstick

- **Fixed mobs getting stuck on 1-block ledges** (the "mobs can't jump between
  blocks" bug): `updatePhysics` reset `m_onGround` to `false` *before* the
  horizontal moves, and resolved the vertical move *after* them. The step-up
  branch in `moveAxis` therefore tested `m_onGround` while it was always false
  and never fired -- it was unreachable code. Vertical is now resolved first, and
  a `m_onGround` probe confirms contact for mobs resting exactly on a surface.
- **Step-up** (`Mob::tryStepUp`): *(superseded in v0.M5.2 by natural physics jumping)* raised the
  whole body by `kStepHeight` (1.05) and re-tested.
- **Jumping** (`Mob::tryJump`): fires when step-up cannot resolve the obstacle
  (2+ blocks, or after panic knockback), with a `kJumpCooldown` so mobs do not
  hop continuously against a wall.
- **Unstick logic** (`Mob::updateStuckResponse`): blocked for `kBlockedJump`
  seconds -> one hop; blocked for `kBlockedReroll` seconds -> pick a new heading
  via `rerollHeading()`. Previously a mob ground against an obstacle until its
  wander timer expired, which is what made the herd look like it was "just
  roaming around".
- **Ledge avoidance** (`Mob::dropAhead`, `AiAvoidEdge`): mobs probe the column
  ~1.3 blocks ahead and refuse to walk off a drop greater than `kSafeDrop`,
  turning away instead. They no longer march into cave mouths and fall.
- **Better swimming** (`AiFloat`): buoyancy now has a surface deadband, so a mob
  already at the waterline stops rising instead of bobbing forever. Water is
  sampled at the feet *and* head height, so a chest-deep mob swims rather than
  walking on the sea bed, and horizontal swimming is damped to 0.6x.
- **AI goal mask** ([`MobAi.hpp`](file:///C:/Users/Ais-PC/VoxelMin/src/entity/MobAi.hpp)):
  `Idle`, `Wander`, `Panic`, `LookAt`, `StepUp`, `Jump`, `AvoidEdge`, `Float`
  are now named bits in an `AiGoalMask` with per-species tables (`kPigGoals`,
  `kCowGoals`). Disabling a behaviour for a species is a one-line table edit.
  Goals are ordered cheapest-test-first.
- **Cached heading maths** (`Mob::updateForwardVector`): `sin`/`cos` of the yaw
  are cached and only recomputed when the heading actually changes, instead of
  per mob per frame.
- Reference and rationale recorded in [`SOURCES.md`](file:///C:/Users/Ais-PC/VoxelMin/docs/SOURCES.md)
  section 4 (BuiltBrokenModding/AI-Improvements).
- Verified over a 4000-frame headless run: 24 mobs, ~22 grounded, cumulative
  step-ups 10 -> 21 -> 36 -> 42 and jumps 4 -> 21 -> 22 -> 29, i.e. mobs
  continuously climb terrain and hop when step-up is not enough.

### 2026-09-28 -- VoxelMin v0.M5.1: GUI Scale setting, shared UI scale, safer menu footer

- Added a `GUI Scale` option (AUTO / 1 / 2 / 3 / 4) to the Options menu.
- The UI scale is now computed in one place, `Application::computeUiScale()`,
  and pushed to `Renderer::setUIScale()` each frame. Both menus and the HUD read
  `Renderer::uiScale()` so they can never disagree.
- Auto scale is based on the smaller framebuffer dimension
  (`min(height, width * 0.6) / 280`, clamped 1-8) instead of the old
  `round(width / 420)`, which jumped too large in fullscreen (e.g. 1920x1080 now
  gives 4 instead of 5).
- Menu layout reserves a bottom safe area (`max(22, 4% of height)`) so the
  footer hint text clears the Windows taskbar, and clamps the button stack so
  the lowest row never overlaps the footer.
- Capture harness gained `VOXELMIN_CAPTURE_SIZE` and
  `VOXELMIN_CAPTURE_GUI_SCALE` for verifying scaling at other resolutions.

### 2026-09-28 -- VoxelMin v0.M5.1 (legacy v0.7.0): 3D Torch Models & Particle System, Water Mechanics, World Types & Custom Name Input

- **3D Standing Torch Models**:
  - Overhauled torch rendering in [`ChunkMesher.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/ChunkMesher.cpp) from flat 2D quads to true 6-faced 3D standing wooden sticks (`emitTorch3D`).
  - Textures stick faces with full oak grain UV mapping.
- **Fire & Smoke Particle System**:
  - Implemented GPU-accelerated particle system in [`ParticleSystem.hpp`](file:///c:/Users/Ais-PC/VoxelMin/src/render/ParticleSystem.hpp) / [`ParticleSystem.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/render/ParticleSystem.cpp) with dedicated shaders ([`particle.vert`](file:///c:/Users/Ais-PC/VoxelMin/assets/shaders/particle.vert), [`particle.frag`](file:///c:/Users/Ais-PC/VoxelMin/assets/shaders/particle.frag)).
  - Camera-facing billboard particles with additive/soft blending and life decay for flickering fire sparks and rising smoke plumes on active torches.
- **Underwater Visual Shader & Fog Overhaul**:
  - Implemented custom underwater post-processing shaders ([`underwater.vert`](file:///c:/Users/Ais-PC/VoxelMin/assets/shaders/underwater.vert), [`underwater.frag`](file:///c:/Users/Ais-PC/VoxelMin/assets/shaders/underwater.frag)) with oceanic blue color filtering, animated caustic shimmer ripples, and a dark radial screen vignette.
  - Automatically activates dense underwater fog (`fogStart = 1.0f`, `fogEnd = 15.0f`) with deep blue clear color (`0.06, 0.18, 0.44`) whenever the camera eye is submerged, dramatically reducing view distance for realistic underwater murkiness.
- **Water Mechanics, Buoyancy & Swimming**:
  - Implemented water swimming physics for player in [`Player.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/player/Player.cpp): holding `Space` swims upward (`4.2 blocks/s`), `Left Shift` dives downward, horizontal swimming has natural fluid drag (`0.70x`), and gravity applies gentle buoyant sinking (`-3.5 blocks/s` cap) rather than falling at land speed.
  - Single liquid type block with non-breakable protection: Left-clicking water passes through to target and dig underwater solid blocks.
  - Implemented fluid surface lip recession (`0.12f`) in `ChunkMesher.cpp` so water surfaces sit lower than surrounding ground blocks.
  - Added `isLiquid()` and `isBreakable()` in [`Block.hpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/Block.hpp) and [`Raycast.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/Raycast.cpp).
- **Mob Mesh Rendering & Backface Culling Fix**:
  - Corrected Top (+Y) and Bottom (-Y) cube face vertex winding orders in [`Mob.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/entity/Mob.cpp) from clockwise to counter-clockwise. All 6 mob box faces now render solid and opaque without missing back/top faces when viewed from any angle.
- **World Type Selection & Procedural Generators**:
  - Added `WorldType` selector in the Create New World menu with 5 generation archetypes in [`TerrainGenerator.cpp`](file:///c:/Users/Ais-PC/VoxelMin/src/world/TerrainGenerator.cpp):
    1. *Default*: Varied hills, lakes, caves, trees, and ore deposits.
    2. *Flat*: Classic superflat bedrock/dirt/grass plane for building.
    3. *Mountainous*: Soaring cliffs, high plateaus, and deep river gorges.
    4. *Cavernous*: Hollow Swiss-cheese world riddled with caves, underground pools, and alcoves.
    5. *Island*: Isolated landmass surrounded by deep oceanic waters.
- **Real-Time World Name Text Input**:
  - Implemented live alphanumeric text input with blinking cursor (`_`) and Backspace character deletion in the Create New World menu using GLFW character input callbacks (`charCallback` / `handleCharInput`).
- **Version Bump**: Bumped to `v0.7.0` (now `v0.M5.1`) across `Config.hpp`, window title, startup logs, menus, and documentation.

### 2026-09-28 -- VoxelMin v0.M5.0 (legacy v0.6.0): Player Sprinting, Dynamic FOV, and Neutral Mobs (Pigs & Cows with AI)

- **Player Sprinting & Dynamic FOV**:
  - Implemented sprinting mechanics in [`Player.cpp`](file:///C:/Users/Ais-PC/VoxelMin/src/player/Player.cpp) activated via holding `Left Control` (or double-tapping `W`).
  - Sprint speed boosted to `7.4 blocks/s` (vs walking speed `5.0 blocks/s`).
  - Implemented dynamic FOV zooming transitioning smoothly from `70°` to `80°` when sprinting (and `85°` during super-speed creative flight).
- **Neutral Mobs & Entity AI Architecture**:
  - Implemented [`Mob`](file:///C:/Users/Ais-PC/VoxelMin/src/entity/Mob.hpp) and [`EntityManager`](file:///C:/Users/Ais-PC/VoxelMin/src/entity/EntityManager.hpp) supporting autonomous passive animal entities.
  - **AI Behaviors**: State machine with `Idle` (standing, tilting head, looking around), `Wander` (directional path exploration, jumping 1-block obstacles), `LookAtPlayer` (curious head tracking when player approaches within 6 blocks), and `Panic` (high-speed erratic evasion when damaged).
  - **AABB Physics & World Collision**: Full axis-by-axis voxel collision, auto-step jumping, and water flotation/swimming.
- **3D Textured Mob Models & Animations**:
  - **Pig Model**: Textured pink body, head, snout with nostril details, and 4 animated walking legs.
  - **Cow Model**: Black and white spotted hide, head with horns, pink udder detail, and 4 animated walking legs.
  - Alternating sinusoidal leg-swing animations synchronized to mob movement velocity.
- **Procedural Mob Pixel Art Textures**:
  - Generated pixel art tiles in [`Texture.cpp`](file:///C:/Users/Ais-PC/VoxelMin/src/render/Texture.cpp): `PigSkin`, `PigFace`, `PigSnout`, `CowSkin`, `CowFace`, and `CowHorns`.
- **Combat & Interaction**:
  - Added raycast hit testing for entities in `Application::updateInteraction()`. Left-clicking deals knockback, triggers hurt flash, and initiates panic AI.
- **Batched Dynamic Mob Rendering**:
  - Implemented `Renderer::drawEntities()` in [`Renderer.cpp`](file:///C:/Users/Ais-PC/VoxelMin/src/render/Renderer.cpp) uploading transformed entity geometry in a single batch with full vertex AO, dynamic skylight/torchlight sampling, directional shading, and distance fog.

### 2026-09-28 -- VoxelMin v0.M3.1 (legacy v0.5.0): Procedural Randomized Trees & Smooth Lighting Overhaul

- **Procedural Randomized Trees**:
  - Overhauled [`TerrainGenerator::plantTree`](file:///C:/Users/Ais-PC/VoxelMin/src/world/TerrainGenerator.cpp) with 4 distinct procedural tree archetypes selected deterministically via seed hash:
    1. *Classic Oak*: Varied height (4-6 blocks), balanced 5x5 rounded canopy, and full leaf dome.
    2. *Tall Forest Oak*: High canopy (6-8 blocks trunk), spherical multi-layered leaf volume.
    3. *Conical Pine / Spruce*: Slender tall trunk (6-8 blocks), tiered conical silhouettes with layered skirt rings and a spire apex.
    4. *Bushy / Apple Tree*: Low trunk (3-4 blocks), dense dome canopy.
  - **Fixed bare tree tops**: Added full leaf crown generation on `topY + 1` and `topY + 2` directly over the central trunk column (`dx=0, dz=0`), completely eliminating bare exposed log holes when viewing trees from above.
- **Smooth Lighting & Soft Ambient Occlusion Polish**:
  - Implemented smooth horizontal skylight diffusion in [`World::skyLight`](file:///C:/Users/Ais-PC/VoxelMin/src/world/World.cpp) across adjacent columns (`radius = 1..2`) to eliminate harsh cliff cutoffs under overhangs and deep pits.
  - Softened contact vertex Ambient Occlusion factors in [`ChunkMesher.cpp`](file:///C:/Users/Ais-PC/VoxelMin/src/world/ChunkMesher.cpp) (`0: 0.60f, 1: 0.75f, 2: 0.88f, 3: 1.00f`).
  - Tuned directional face shading and ambient light floor in [`assets/shaders/chunk.frag`](file:///C:/Users/Ais-PC/VoxelMin/assets/shaders/chunk.frag) (`lightFloor = 0.35`, soft AO factor curve), ensuring natural atmospheric contrast in shadows and caves without crushed pitch-black artifacts.

### 2026-09-28 -- VoxelMin v0.M3.1 (legacy v0.5.0): External Splash Text Data File & Randomization

- Bumped minor version to `v0.5.0` across `Config.hpp`, window title, startup logs, and main menu.
- **External Splash Text Data Pipeline**:
  - Created [`assets/splashes.txt`](file:///C:/Users/Ais-PC/VoxelMin/assets/splashes.txt) with 36+ curated retro voxel & Minecraft-inspired splash phrases.
  - Implemented `loadSplashes()` in `Application.cpp` with multi-path asset resolution and clean fallback defaults.
  - Implemented `getRandomSplash()` using `std::mt19937` to randomize the animated yellow title splash on each launch and whenever returning to the main menu.
- **Lighting & Smooth Ambient Occlusion**:
  - Implemented vertical skylight / sunlight column propagation in `World::skyLight` with attenuation under trees and overhangs.
  - Implemented 4-level vertex Ambient Occlusion (`computeVertexAO`) in `ChunkMesher.cpp` evaluating 3 corner neighbor occluders per vertex for smooth contact shadows.
  - Implemented diagonal flipping on quads (`ao[0] + ao[2] < ao[1] + ao[3]`) to prevent anisotropic triangulation artifacts.
  - Integrated per-vertex `ao` and `light` into `Vertex` struct, `chunk.vert`, and `chunk.frag` combined with directional face shading.
- **Legacy Console Creative Inventory Redesign**:
  - Overhauled in-game inventory modal in `Renderer::drawInventory` to authentic Legacy Console style (Xbox 360 / PS3 Edition).
  - Designed 3D sunken slot bevels, distinct panel framing, gold `"CREATIVE INVENTORY"` tab header, and engraved divider for the Hotbar section.
  - Added full-screen background dimming, glowing hover outlines, active hotbar slot indicator, floating dark-purple tooltip boxes, and held-item cursor shadows.
- **Game Window Icon**:
  - Created 32x32 pixel art grass block application icon (`assets/icon.png`).
  - Integrated runtime window icon loader in `Application::init()` using `glfwSetWindowIcon`.
- **16x16 Pixel Textures & Seamless Texture Derivatives**:
- Added a `stb_image` PNG loader to `Texture::createAtlas()` with a
  per-tile procedural fallback, and integrated 16x16 pixel textures from
  `assets/textures/` for the blocks that have them (10 of 17 slots are filled;
  ores, cobblestone, torch and water still render from the procedural
  generators -- see `Known limitations`).
- Configured 256x256 texture atlas (16x16 grid of 16x16 tiles) in `Config.hpp` and `Texture.cpp`.
  - Replaced naive texture wrapping in `chunk.frag` with hardware gradient-driven `textureGrad(uAtlas, uv, dFdx(vUV) * vTileSize, dFdy(vUV) * vTileSize)` to completely eliminate mipmap derivative jumps and dark seam lines across greedy-meshed quad boundaries.
- **2D Sprite Icon Renderer for UI**:
  - Created `sprite.vert` and `sprite.frag` shaders and integrated dynamic 2D sprite rendering (`drawBlockIcon` / `drawTexturedRect`) in `Renderer.hpp` / `Renderer.cpp`.
  - Updated HUD Hotbar, 24-slot Storage Inventory grid, and cursor-held drag items to render crisp 16x16 textured block sprites instead of flat color boxes.
- **Pause Menu Text Fix**:
  - Added missing `&` glyph entry to `FontData.hpp` and standardized pause menu button to `"Save and Quit to Title"` in `Application.cpp`.
- **Greedy Meshing Algorithm**:
  - Replaced naive per-voxel quad meshing with slice-based 2D Greedy Meshing in `ChunkMesher.cpp`.
  - Merges coplanar identical adjacent faces along all 6 cube directions (+X, -X, +Y, -Y, +Z, -Z).
  - Implemented shader-side fractional UV atlas tiling (`fract(vUV) * tileSize + tileMin`) in `chunk.vert` / `chunk.frag`, eliminating atlas bleeding on arbitrary merged quads.
- **View Frustum Culling**:
  - Implemented 6-plane Gribb-Hartmann frustum extraction in `Frustum.hpp`.
  - Added Chunk AABB intersection checks in `Renderer::drawWorld` to skip non-visible chunk draw calls.
- **Dual-Pass Rendering & Transparency / Cutout Pass**:
  - Chunk geometry split into `opaqueMesh` and `transparentMesh` streams.
  - Pass 1: Opaque blocks with depth write and backface culling enabled.
  - Pass 2: Transparent/Cutout blocks (leaves) with two-sided rendering and alpha discard in shader.
- **Sources & References Documentation**:
  - Created `docs/SOURCES.md` tracking all external open-source references (FerriteCore, ImmediatelyFast, VanillaHD-Mosaic, and algorithmic papers).

### 2026-09-28 -- VoxelMin v0.M4.0 (legacy v0.4.0): Named Worlds, World Creation Menus, Water, Caves & Torches

- **World Selection & Creation Menus**:
  - Implemented authentic Legacy-Console-style **Select World** menu listing all existing saved worlds in `saves/` with names and seeds.
  - Implemented **Create New World** menu supporting configurable world names, custom seed input, and a "Roll Random Seed" generator.
- **Named World Persistence (`saves/<world_name>.dat`)**:
  - Upgraded save format to Version 2 in [`WorldSave.hpp`](file:///C:/Users/Ais-PC/VoxelMin/src/world/WorldSave.hpp) / [`WorldSave.cpp`](file:///C:/Users/Ais-PC/VoxelMin/src/world/WorldSave.cpp) with embedded world name, seed, and automatic `saves/` folder management while retaining backward compatibility with legacy saves.
- **Water & Coastal Beaches**:
  - Added `BlockId::Water` with translucent texture and alpha blending in the transparent pass.
  - Integrated sea level filling and natural sandy beaches along lakeshores and coastlines in [`TerrainGenerator.cpp`](file:///C:/Users/Ais-PC/VoxelMin/src/world/TerrainGenerator.cpp).
- **3D Procedural Caves & Mineral Ores**:
  - Implemented 3D noise cavern and worm tunnel carving (`noise3D`).
  - Added mineral ore veins embedded in underground stone: `CoalOre`, `IronOre`, `GoldOre`, and `DiamondOre`.
- **Torches & Point Block Lighting**:
  - Added `BlockId::Torch` with custom sprite and non-solid physical properties.
  - Implemented dynamic 3D block light propagation in [`World::skyLight`](file:///C:/Users/Ais-PC/VoxelMin/src/world/World.cpp) and multi-chunk dirtying on torch placement/breaking for instant cave illumination.

### 2026-09-28 -- VoxelMin v0.M3.0 (legacy v0.2.0): World Save/Load & Non-Freezing Inventory

- Bumped version to `v0.2.0` (now `v0.M3.0`) across `Config.hpp`, window title, startup logs, and main menu.
- **World Save & Load Persistence (`world.dat`)**:
  - Implemented binary save file format (`VOXS`, version 1) in `WorldSave.hpp` / `WorldSave.cpp`.
  - Serializes world seed, chunk grid dimensions, player position/yaw/pitch/flight state, hotbar (8 slots), storage inventory (24 slots), selected slot, and RLE-compressed chunk block streams.
  - Chunk blocks saved using Run-Length Encoding (`RLERun`). At the time of v0.2.0 this shrank a full 128x80x128 world to ~61 KiB on disk; the v0.M5.1 world (caves, ores, water) is 343 KiB -- see `Current metrics`.
  - Automatic world loading on startup (`Application::init`) if `world.dat` exists.
  - "Save & Quit to Title" menu action and auto-save on shutdown.
- **Non-Freezing Real-Time Inventory**:
  - Opening the Storage Inventory (`E` key) keeps the world running in real time (physics, momentum, gravity, chunk meshing continue).
  - Player look is automatically locked while cursor is uncaptured in inventory mode; keyboard movement is safely gated while allowing gravity, jumping momentum, and falling to resolve smoothly.
  - Pausing with `Esc` (Pause Menu / Options) continues to freeze world simulation.

### 2026-09-28 -- VoxelMin v0.M2.1 (legacy v0.1.2): Storage Inventory Modal

- Bumped version to `v0.1.2` (now `v0.M2.1`) in `Config.hpp`, window title, startup log, and main menu subtitle.
- **Storage Inventory Modal**:
  - Toggled in-game by pressing `E` (or `Esc` / `E` to close). Automatically uncaptures mouse cursor while open.
  - Features a 24-slot Storage Inventory Grid (3x8) and an 8-slot Hotbar Grid.
  - Full item management: left-click to pick up/swap blocks between storage slots, hotbar, and cursor.
  - Hover tooltips display block names (e.g. Grass, Dirt, Stone, Wood, Leaves, Sand, Bedrock, Planks) and slot numbers.
  - Number keys `1-8` while hovering any inventory slot directly assign that block to hotbar slot `1-8`.

### 2026-09-28 -- VoxelMin v0.M2.0 (legacy v0.1.1): Compact crosshair & bidirectional options control fix

- Bumped version to `v0.1.1` (now `v0.M2.0`) in `Config.hpp`, window title, startup log, and main menu subtitle.
- **Compact Crosshair**: Redesigned the HUD crosshair in `Renderer::drawHud` to scale compactly across all GUI scales (arms scaled down from 24px to 3-6px with a high-contrast dark outline + bright inner core).
- **Bidirectional Option Controls**:
  - Fixed options menu adjustment (FOV, View Distance, Mouse Sensitivity, GUI Scale) which previously only increased values when clicked with mouse.
  - Left-clicking left half of option row or right-clicking decreases value (-1); left-clicking right half increases value (+1).
  - Selected options display visual indicators (`< value >`).

### 2026-09-28 -- VoxelMin v0.M1.0 (legacy v0.1.0): Game versioning & FerriteCore-inspired palette voxel memory optimization

- Introduced official version `v0.1.0` (now `v0.M1.0`) in `Config.hpp` (`VOXELMIN_VERSION`), window title, startup log, and main menu subtitle.
- Implemented `ChunkPalette` (FerriteCore-inspired memory optimization):
  - Uniform single-block chunks (e.g. empty air columns above terrain) consume 0 index array bytes (1 byte palette).
  - Multi-block chunks use indexed 8-bit palette storage (`ChunkPalette`) to minimize memory footprint.
  - `World::totalVoxelMemory()` calculates and logs live voxel RAM usage on startup.
- Added `RLERun` stream compression utility (`Chunk::rleCompress()`) for run-length encoded save files and network serialization.

### 2026-09-28 -- VoxelMin v0.M2.0 (M2 milestone delivery): Legacy-console-style menus

- Game state machine: `MainMenu`, `Playing`, `Paused`, `Options`.
- Main menu over a rotating world panorama: logo with shadow + green underline,
  animated yellow splash text, subtitle, footer control hints.
- Pause menu ("Game Menu") over a dimmed, frozen world: Back to Game / Options /
  Quit to Title.
- Options screen with live settings: Mouse Sensitivity, Field of View, View
  Distance (fog), VSync, Fullscreen, Wireframe. Navigated with Up/Down + Enter
  and Left/Right to change values; mouse hover/click also works.
- Procedural 5x7 bitmap font covering punctuation, digits and A-Z/a-z built at
  startup (no font assets). `Renderer` gained 2D primitives: `drawRect`,
  `drawGradientRect`, `drawTriangle`, `drawText`, plus a `text` shader.
- Legacy-inspired UI style: dark beveled rows, light top highlight, gold
  selection outline + left marker; background dimming and panorama camera.
- `Esc` now opens/closes the in-game menu; cursor is captured only while playing.
- Environment-driven screenshot capture (`VOXELMIN_CAPTURE`) added for
  verification, with `menu`/`play`/`pause`/`options`/`pause_options` states.

### 2026-09-28 -- Pre-versioning (M1): project bootstrapped, toolchain set up

- Toolchain set up (MSYS2 MinGW-w64 GCC 16.2, CMake 4.4, Ninja 1.13, GLFW 3.5,
  GLEW 2.3, GLM 1.0.3). Executable links statically -- no runtime DLLs needed.
- Window + OpenGL 3.3 Core context, vsync, resize handling.
- Fixed chunked world: 8x8 chunks, 16x80x16 blocks each (128 x 80 x 128 total).
- Seeded value-noise fBm terrain with stone/dirt/grass layering and trees.
- Procedural texture atlas (10 tiles) generated in code.
- Per-face culled chunk meshing, world-space vertices, incremental rebuilds.
- First-person mouse-look camera.
- Player physics: gravity, jump, axis-by-axis AABB collision, wall sliding.
- Creative flight toggle.
- Voxel DDA raycast for block targeting, with a selection outline.
- Break (left click) and place (right click) blocks; 8-slot hotbar.
- Fog, per-face shading, crosshair + hotbar overlay.
- Wireframe debug view (`G`).
- Documentation: README, ARCHITECTURE, PROGRESS, ROADMAP, HANDOFF, BUILD, AGENTS.

## Infrastructure

Dev-tooling and build changes, deliberately kept out of the version chain above
because they are not player-facing. The version history tracks shipped
*features* only.

- IDE and compiler-database setup (`.clangd`, `.vscode/c_cpp_properties.json`,
  and the root `compile_commands.json` refreshed by `scripts/build.ps1`) is
  documented once, in [`BUILD.md`](BUILD.md) -> "IDE & compiler database". Do not
  restate it here; duplicated prose drifts.
- `Application.hpp` includes `<glm/vec3.hpp>` explicitly so its `glm::ivec3`
  members stay self-contained for indexers. Do not remove it even though it
  looks redundant to the compiler.

## Known limitations

These are intentional for the current milestone and tracked in `docs/ROADMAP.md`.

- Fixed world size (128 x 80 x 128 blocks); no infinite chunk streaming, no background generation.
- No audio engine (ambient sounds, block break/place, footsteps, menu sounds).
- No item drops or item entities, no crafting grid, no tools with durability.
- No hostile mobs or mob spawn/cap system; current mobs are neutral and hand-placed by the spawner.
- No day/night cycle (static daytime directional sunlight).
- Menus are keyboard/mouse only (no controller/D-pad support, no key rebinding).
- Collision uses full-axis revert, leaving a sub-millimetre gap when resting against geometry.
- Block textures are incomplete: 7 of the 17 atlas slots have no PNG
  (`coal_ore`, `cobblestone`, `diamond_ore`, `gold_ore`, `iron_ore`, `torch`,
  `water`) and fall back to procedural generators, so those blocks do not match
  the shipped texture style yet.
- Single-threaded meshing; spikes can occur when many chunks are dirtied simultaneously (dampened by frame budget).
- No automated test suite and no CI build pipeline.

## Verification checklist

1. `pwsh -File scripts/build.ps1` -> finishes with no warnings or errors.
2. `pwsh -File scripts/run.ps1` -> logs show:
   - `Renderer initialised (OpenGL 3.3.0 ...)`
   - `Font atlas built: ...`
   - `VoxelMin v0.M5.2 starting...`
   - `World ready: 128 x 80 x 128 blocks (voxel RAM: ... KiB)`
3. Main menu over a rotating panorama; splash text differs between launches.
4. `Select World` lists saves in `saves/`, allows `Del` key or button deletion with a confirmation dialog; `Create New World` accepts a typed name and offers 5 world types.
5. In game:
   - WASD + mouse look. Sprint with Left Ctrl (or double-tap W); FOV widens.
   - Dynamic view bobbing sways and bounces with footsteps, accentuating during sprinting with subtle strafe roll tilting.
   - Source-style movement: air-strafing with A/D adds momentum, holding Space chains frictionless bunnyhops.
   - Break with left click, place with right click; hotbar 1-8 selects.
   - `E` opens the storage inventory; drag items; `Esc` closes; **the world
     keeps running** while it is open (this is the M3 non-freezing rule).
   - Place a torch -> 3D stick model, compact 0.25x0.65x0.25 hitbox, flame/smoke particles, local light.
   - Wild tall grass scatters across meadows with double-sided cross quads and stable cutout rendering.
   - Enter water -> swim up with Space, dive with Shift, underwater shader.
   - Approach a pig -> head tracks the player; hit it -> panic knockback.
     Mobs jump naturally over 1-block steps via parabolic arcs rather than teleporting.
   - `F3` toggles the overlay: frame time, fps, draw calls, triangles, chunk
     visible/drawn/culled. `chunks drawn + culled` should equal 64.
   - `G` toggles wireframe; `Esc` opens the Game Menu; `Options` changes
     settings (including GUI Scale Auto/1-4).
6. `Save and Quit to Title` writes `saves/<world>.dat`. Relaunch -> the world
   restores block edits and the player position.

The menus can also be captured non-interactively:

```powershell
$env:VOXELMIN_CAPTURE="menu.bmp"; $env:VOXELMIN_CAPTURE_STATE="menu"
build\bin\VoxelMin.exe
```

Add `$env:VOXELMIN_CAPTURE_VSYNC="0"` and read the `Stats:` line the capture
prints to check performance numbers without vsync capping them.

## Regression checks

The verification checklist above proves the game works. These catch the failure
modes that a refactor introduces while the game still appears to work. Run them
after any change to the world, mesher, or renderer.

- [ ] `chunks drawn + culled` equals 64 at spawn (catches double-tested or
      skipped frustum culling; this invariant was violated before the v0.M5.1
      culling fix).
- [ ] `chunks culled` is non-zero when facing a wall.
- [ ] Triangles/frame has not regressed against the `Current metrics` baseline.
- [ ] Save -> reload -> block edits and player position persist.
- [ ] Placing a torch propagates light immediately; breaking it removes the
      light and its particles.
- [ ] Torch selection wireframe highlights only the 0.25x0.65x0.25 stick/flame area,
      allowing raycasts through surrounding voxel air to reach background blocks.
- [ ] Wild tall grass renders without flicker/z-fighting at any camera angle (this
      regressed in v0.M5.1 and was fixed in v0.M5.2 by removing duplicate coplanar quads).
- [ ] Mobs clear a 1-block step with physics jumping without visible teleportation
      (step-up was removed in favour of physics jumping in v0.M5.2).
- [ ] Holding Space smoothly chains frictionless bunnyhops preserving air-strafe speed.
- [ ] The underwater shader engages only when the eye is submerged, not when
      the feet are in water.
- [ ] Breaking a torch returns the particle count to baseline.
- [ ] Editing a block on a chunk border dirties the neighbour chunk too, and
      each dirty chunk is rebuilt once rather than twice.
- [ ] No GL errors after a full frame: `glGetError()` stays `GL_NO_ERROR`.

## v1.M2.0 (2026-10-02) -- The Backrooms Dimension, TrueType Typography & FastNoiseLite

### Highlights
- **The Backrooms Level 0 ("The Yellow Hell")**:
  - Infinite procedural non-linear partition maze with mono-yellow wallpaper, moist damp carpet floor, and acoustic ceiling tiles.
  - Symmetrical fluorescent ceiling light fixtures with humming ambient diffuse illumination.
  - Random dark blackout zones and hallway clusters generated via continuous OpenSimplex2 noise.
  - Reality Glitch blocks (`GlitchBlock`) allowing instant noclip travel into the dimension.
  - Rare Emergency Fire Exit doors (`ExitDoor`) and Almond Water item caches (`AlmondWater`).
  - Safe dimension transition offsets and 2-second cooldown to prevent re-teleportation loops.
  - Custom analog VHS noise shader, voltage flicker, CRT scanlines, and liminal dark edge vignette.
  - Unsettling ambient synthesized soundtrack for Backrooms with dynamic audio crossfade.
  - RPG-style cinematic dimension entry banners ("LEVEL 0 - The Yellow Hell").
- **TrueType / OpenType Typography Engine (`born2bsporty-fs.otf`)**:
  - Integrated `stb_truetype` rasterization with precise vertical ascent and downward glyph offset calculations.
  - Automatic fallback to embedded procedural bitmap font atlas if external font files are missing.
  - Restructured Main Menu and pause screen button layouts with centered labels.
- **FastNoiseLite & OpenSimplex2 Integration**:
  - Integrated header-only `FastNoiseLite.hpp` for continuous multi-octave simplex/perlin noise world generation.
- **Blockbench MCP Integration**:
  - Integrated headless Blockbench MCP tooling for 3D voxel item, block, and mob modeling workflows.

| Concern | File |
|---------|------|
| Game loop, state machine, input dispatch, capture harness | `src/core/Application.cpp` |
| World selection/creation/deletion menus | `src/core/Application.cpp` (`buildMenus`, `handleMenuInput`) |
| Tunables and the `VERSION` constant | `src/core/Config.hpp` |
| Rendering, UI primitives, all draw paths, F3 overlay | `src/render/Renderer.cpp` |
| Camera roll, view matrix and perspective projection | `src/render/Camera.{hpp,cpp}` |
| Per-frame and rolling render counters | `src/render/FrameStats.{hpp,cpp}` |
| View frustum extraction and chunk AABB test | `src/render/Frustum.hpp` |
| Particles | `src/render/ParticleSystem.{hpp,cpp}` |
| Greedy chunk meshing, vertex AO, 3D torch and cross foliage geometry | `src/world/ChunkMesher.cpp` |
| Terrain, trees, caves, ores, world types, tall grass | `src/world/TerrainGenerator.cpp` |
| Backrooms Level 0 infinite maze generation | `src/world/BackroomsGenerator.{hpp,cpp}` |
| Skylight and blocklight propagation | `src/world/World.cpp` |
| Save format, directory listing (`listSavedWorlds`) and deletion (`deleteWorld`) | `src/world/WorldSave.{hpp,cpp}` |
| Block ids, tile mapping, solids, block bounds, hotbar colours | `src/world/Block.hpp` |
| Voxel DDA raycast & sub-block AABB tests | `src/world/Raycast.cpp` |
| Player physics, Source movement, air-strafing, bunnyhop, view bobbing | `src/player/Player.cpp` |
| Mob physics, models, traversal, physics jumping | `src/entity/Mob.cpp` |
| AI goal masks and tuning constants | `src/entity/MobAi.hpp` |
| Mob list, spawning, entity raycast, batching | `src/entity/EntityManager.{hpp,cpp}` |
| Menu model and rendering | `src/ui/Menu.cpp` |
| TTF/OTF & Bitmap font rasterization | `src/render/Font.cpp`, `src/render/FontData.hpp` |

## See also

- [`ROADMAP.md`](ROADMAP.md) -- what ships next, and what is out of scope.
- [`REFACTOR_PLAN.md`](REFACTOR_PLAN.md) -- codebase structuring and modularization plan.
- [`ARCHITECTURE.md`](ARCHITECTURE.md) -- how the code fits together.
- [`SOURCES.md`](SOURCES.md) -- external references and library choices.
- [`BUILD.md`](BUILD.md) -- toolchain setup and build commands.
- [`HANDOFF.md`](HANDOFF.md) -- notes for a future maintainer.

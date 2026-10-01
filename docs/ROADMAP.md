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

## M5 -- Gameplay & Entities (In Progress)

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
- **Item entities & drops**: block break drops, physics item pickups.
- **Crafting & Tools**: 2x2 / 3x3 crafting grid, pickaxe/axe/shovel tool durability and mining speed multipliers.

**M5 closes when:** item entities (block break drops + pickup physics) and the
crafting grid + tools with durability land. Nothing else remains in M5. Audio
and infinite chunks were moved to M6 specifically so M5 can actually complete.

Cut **v0.M5.5** when both land; v0.M5.5 will be the final M5 release unless a
bugfix forces a v0.M5.6.

## M6 -- Audio & World Streaming (Next, not started)

Two features that share a milestone because both are "the world keeps running
while the player moves" problems. The design questions that must be answered
first are listed in `Open decisions (blocking M6)` in `docs/PROGRESS.md`.

**Acceptance criteria -- streaming:**
- [ ] Chunks generate, mesh, load and unload within a configurable radius.
- [ ] Meshing runs off the main thread; no frame exceeds 16 ms while streaming.
- [ ] Walking 1000 blocks in one direction never blocks a frame.
- [ ] Existing VOXS v2 saves either migrate to v3 or are rejected with a clear
      message. Never fail to load silently.
- [ ] `F3` shows streaming stats: queued / meshing / ready chunk counts.

**Acceptance criteria -- audio:**
- [ ] Backend chosen, linked and initialised; clean shutdown with no leaks.
- [ ] Footsteps, block break, block place and menu clicks all play.
- [ ] Ambient wind loop is seamless (no click or gap at the loop point).
- [ ] Master and SFX volume sliders in the Options menu, persisted with settings.

**Risks:**
- Threading introduces data races. Budget real debugging time, and keep the
  single-threaded path working behind a flag so a race is always bisectable.
- Save v3 is a breaking format change. Decide migrate-vs-reject before writing
  the streaming code, not after.
- The audio backend choice changes the build system and MSYS2 dependencies.

**Non-goals for M6:**
- Biomes (needs generation work that is not streaming work).
- Music tracks.
- Positional / 3D audio (mono or simple stereo panning only).

## M7 -- Engine Quality (Partially started)

**Partial: the F3 profiling overlay landed in v0.M5.1, ahead of schedule.**
Everything else below is still not started.

- Automated tests (mesh validation, raycast unit tests, terrain determinism). *Not started.*
- CMake `install`/packaging, CI build on push. *Not started.*
- ~~Profiling overlay~~ (**Done, pulled forward into v0.M5.1**): `F3` shows frame
  time, FPS, draw calls, triangles, chunk visible/drawn/culled, mobs and
  particles. Measured baseline is in the `Current metrics` table in
  `docs/PROGRESS.md`. Still missing: real GPU timing via `glFinish()` or
  `GL_TIME_ELAPSED` queries, since the current frame time is CPU-side only.
- Config file and command-line arguments (`--seed`, `--width`, `--height`). *Not started.*

## Explicitly out of scope

Not "not now" -- out of scope for this project. Do not schedule these.

- **Multiplayer.** The project goal is a single-player voxel sandbox.
- **Redstone / complex block logic** (pipes, doors, power, contraptions).
- **Modding API.**
- **Mobile or console ports.**
- **Full survival progression** (Nether, End, ender dragon, boss mobs).

## Deferred

Worth doing, but deliberately not scheduled. Each names what it waits on.

- **Day/night cycle** with a moving sun and coloured fog. Waits on sky rendering
  plus a second lighting pass over the whole world.
- **Weather** (rain/snow). Depends on the particle system, which now exists, so
  this is the cheapest item in this list.
- **Structural generation** (villages, dungeons).
- **Biome-dependent ambient audio.** Waits on M6 audio *and* biomes.
- **Hostile mobs** and a mob spawn/cap system. Depends on the M6 threading model
  for spawning outside the view frustum.
- **Item persistence and mob persistence** -- see the open decisions in
  `docs/PROGRESS.md`.

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

# Architecture

This document explains how VoxelMin is put together. Read it before changing
anything non-trivial.

## High-level

```
                 +---------------------+
                 |     Application     |  window, loop, game state machine
                 +----------+----------+
                            |
        +-----------+-------+-------+-------------+
        |           |               |             |
      Input       World          Renderer        Menu
   (polled)   (chunks/blocks)  (GL resources)  (rows + actions)
                    |               |
                 Player           Font
             (physics/Camera)   (bitmap text)
```

`Application` is the composition root. It creates all subsystems, owns the
`GameState`, and drives the per-frame order; subsystems do not reference each
other except through the dependencies shown above.

## Game states

`GameState` drives what updates and what is drawn:

| State | Player updates | Cursor | Background | Overlay |
| --- | --- | --- | --- | --- |
| `MainMenu` | no | visible | rotating menu camera | main menu |
| `SelectWorld` | no | visible | rotating menu camera | world selection menu |
| `NewWorld` | no | visible | rotating menu camera | create world menu |
| `DeleteWorld` | no | visible | rotating menu camera | delete world menu |
| `ConfirmDeleteWorld` | no | visible | rotating menu camera | confirm delete dialog |
| `Playing` | yes | hidden | player camera | HUD |
| `Inventory` | yes (physics/gravity) | visible | live player camera (dimmed) | storage inventory |
| `GameOver` | no (corpse physics only) | visible | frozen player camera | death screen; world keeps simulating behind it |
| `Paused` | no | visible | frozen player camera (dimmed) | pause menu |
| `Options` | no | visible | depends on where it was opened | options menu |

`Application::activeMenu()` maps the state to a `Menu`. `SelectWorld`, `NewWorld`, `DeleteWorld` and `ConfirmDeleteWorld` are the world-management states; the confirm step exists so a mis-clicked Del cannot destroy a save. Escape pauses/resumes;
`E` toggles the storage inventory without freezing world physics; `Play Game`/`Back to Game` capture the cursor; `setCursorCaptured()` is the only
place that changes GLFW cursor mode.

`Inventory` is the only state that is both a **live world** and an **open UI**:
physics, gravity, momentum, chunk meshing and torch particles keep running while
the modal is up, and mouse-look is suppressed but keyboard movement is safely
gated so the player still falls and settles. `Paused` and `Options` freeze the
simulation. Preserve this distinction when adding new states.

## Frame loop

`Application::run()` performs, in order:

1. `m_input.newFrame()` -- clears per-frame edges/deltas.
2. `glfwPollEvents()` -- callbacks fill `Input`.
3. Query framebuffer size; `Renderer::setViewport()`.
4. Update branch:
   - `Playing`: handle play input (`F`, hotbar), `Player::update`, interaction.
     Escape pauses instead.
   - `Inventory`: handle inventory UI input (`E`/`Esc` to close, item slot pickup/swap), `Player::update` (advances physics/gravity without mouse look).
   - menus: advance the panorama camera and run `handleMenuInput()`.
5. `rebuildDirtyMeshes()` (runs in every state so the world can finish meshing
   behind the main menu or live inventory).
6. `render()` and `glfwSwapBuffers()`.

The order matters: input edges are only valid on the frame they occurred, and
the player must move before the camera is used for raycasting/rendering.

## Coordinates

- **World**: integer block coordinates, `x,z >= 0`, `y >= 0`. The world is
  `WORLD_CHUNKS_X * CHUNK_SIZE_X` blocks wide etc.

> The world is currently fixed at `x,z >= 0` (M5). M6 introduces infinite
> streaming, at which point the coordinate range becomes unbounded and chunk
> lookup must use **floor division** so negative chunk coordinates index
> correctly. `World::chunkAt()` and the `Frustum` AABB math both assume
> non-negative coordinates today and will need revisiting.
- **Chunk local**: `0 <= x < 16, 0 <= y < 80, 0 <= z < 16`.
- **Chunk grid**: chunk `(cx, cz)` covers blocks `[cx*16, cx*16+16)` in X.
- `World::getBlock` is the only cross-chunk accessor; it clamps out-of-range to
  `Air`, and `y < 0` to `Bedrock`.
- The player's `m_position` is the **centre of the feet**; the collision box is
  0.6 wide, 1.8 tall. The camera sits `PLAYER_EYE_HEIGHT` (1.62) above the feet.

## World storage

`Chunk` stores voxel data via `ChunkPalette`, an optimized storage container (inspired by FerriteCore):
- **Single-block mode**: Uniform chunks (e.g. 100% Air or Stone) consume 1 byte (0 index array bytes).
- **Multi-block mode**: Uses an indexed 8-bit palette array + unique block table for compressed RAM footprint.
- **RLE Compression**: `rleCompress()` serializes voxel data into `RLERun` runs for efficient world saves and network payloads.

Index order is `(y * D + z) * W + x`.

`World` owns a `std::vector<std::unique_ptr<Chunk>>` in row-major chunk order.
`World::setBlock` marks the containing chunk dirty and also the neighbouring
chunk if the edited block lies on a chunk border (because that neighbour's
visible faces may change). `World::totalVoxelMemory()` calculates live voxel RAM usage.

## Terrain generation

`TerrainGenerator` is seeded and deterministic: the same seed and world type
always produce the same world. Height comes from value-noise fBm
(`hash`-seeded, multi-octave), with stone/dirt/grass layering and a sea level.

Five **world types** (`WorldType` in `TerrainGenerator.hpp`) select a parameter
set: `Default`, `Flat`, `Mountainous`, `Cavernous`, `Island`. The create-world
menu exposes them; the type is stored in the save file.

- **Caves**: 3D noise carves caverns out of the solid stone, and rejects cells
  above a noise floor so the surface stays intact.
- **Ores**: coal, iron, gold and diamond veins are placed as blobs in stone,
  each with its own depth band, count and rarity, drawn from the same seeded RNG.
- **Trees**: four archetypes (`ClassicOak`, `TallForestOak`, `ConicalPine`,
  `BushyApple`) selected by a deterministic seed hash per surface column, with
  full leaf crowns over the trunk column so tree tops are never bare.

## Lighting

Two independent channels are propagated per block:

- **Skylight** (`World::skyLight`): a vertical column is lit from the sky down,
  attenuating through solid blocks, then diffused horizontally across adjacent
  columns (radius 1-2) so overhangs and cave mouths fall off smoothly instead of
  producing hard edges.
- **Blocklight** (`World::blockLight`): emitted by torches and diffused
  radially, reaching up to 7 blocks.

The mesher bakes both channels plus a 4-level vertex ambient occlusion term into
each vertex (`computeVertexAO`, with a diagonal-flip rule to avoid anisotropic
triangulation artifacts). Because light is stored in the voxel grid, placing or
breaking a block dirties the affected chunks and the light is recomputed during
the next rebuild; `Renderer` samples it per vertex for mob and particle geometry
too, so dynamic entities are lit by the same rules as the world.

## Meshing

`buildChunkGeometry()` iterates every block and, for each of the 6 faces whose
neighbour is not opaque, emits a quad. Positions are written in **world space**,
so the renderer needs no per-chunk model matrix.

Winding is derived from a per-face `(normal, tangent, bitangent)` table where
`tangent x bitangent == normal`; vertices are emitted with counter-clockwise
front faces (OpenGL default `GL_CCW`, back-face culling on).

### Greedy Meshing
`ChunkMesher::buildChunkGeometry()` runs a 2D slice-based greedy meshing algorithm
along all 3 axes (+X, -X, +Y, -Y, +Z, -Z). It merges adjacent coplanar faces
sharing the same block type and transparency state into single large quads.
Fractional UV coords (`fract(vUV)`) combined with tile bounds (`tileMin`, `tileSize`)
in the shader allow merged quads of any dimensions to tile seamlessly without atlas bleeding.

## Texture atlas

Generated at startup by `Texture::createAtlas()` into a **256x256 RGBA buffer**:
16x16 tile slots of **16x16 px** each (`config::ATLAS_TILES`,
`config::TILE_PIXELS`, `config::ATLAS_PIXELS` in `src/core/Config.hpp`).

Tiles are filled by `loadTilePng()`, which decodes `assets/textures/*.png` with
`stbi_load` and rescales to 16x16 with `stbir_resize_uint8_linear` if the source
is a different size. **Every tile has a procedural fallback**: if the file is
missing or the decode fails, `createAtlas()` falls through to the matching
`make*()` generator (noise, grass stripes, ore speckle, water alpha, torch
pixels) so the game still renders. 22 tiles are defined, including entity skins
(`PigSkin`, `PigFace`, `PigSnout`, `CowSkin`, `CowFace`, `CowHorns`) that are
generated procedurally only.

> **Current state:** 10 of the 17 referenced PNGs exist. `coal_ore.png`,
> `cobblestone.png`, `diamond_ore.png`, `gold_ore.png`, `iron_ore.png`,
> `torch.png` and `water.png` are **absent and silently fall back to the
> procedural generators**, so those blocks do not look like their textures yet.
> The fallback is by design; the missing files are an art gap, not a bug.

Biome tinting is applied to grass top and leaves. The atlas generates full
mipmaps and uses `GL_NEAREST_MIPMAP_LINEAR` minification with `GL_NEAREST`
magnification for retro crispness without distance shimmer.

`BlockDef` maps each block's top/side/bottom faces to a `TextureTile`.

## Rendering

Seven shader programs, all loaded from `assets/shaders/*.vert|frag` at startup
via `Shader::loadFromFiles()`:

- `chunk` -- world geometry with greedy UV tiling and alpha cutout discard (`if (texel.a < 0.1) discard;`).
  Per-face shading from the normal (top bright, bottom dark) plus distance fog toward the sky colour. Uniform `uVP` only.
- `skydome` -- camera-centred UV-sphere dome drawn first in `Renderer::DrawSky` with depth writes off.
  The fragment shader maps the view direction to a zenith/horizon/ground gradient derived from the time-of-day
  sky colour, so the sky changes with the direction the player faces instead of being a flat clear colour.
  The horizon colour matches the fog colour so distant terrain blends seamlessly into the sky.
  The dome is world-aligned and follows the camera position, and `Renderer::drawSky` must receive the same
  camera as the world draw (player camera during play, menu camera in menus) or the sky locks to a stale view.
- `line` -- the selection outline. A unit-cube wireframe drawn per targeted block.
- `ui` -- flat 2D geometry with per-vertex colour, in screen pixels; used for
  panels, button fills, gradients, the crosshair and the hotbar.
- `text` -- samples the bitmap font atlas and multiplies it by a uniform colour.
- `sprite` -- textured 2D quads, used for block icons in the hotbar and storage
  inventory slots (16x16 item sprites).
- `particle` -- camera-facing billboard quads for torch flame and smoke, with
  additive/soft blending and life decay. Owned by `ParticleSystem`, not `Renderer`.
- `underwater` -- full-screen tint/caustic filter applied when the camera eye is
  submerged, with dense blue fog.

### Frustum Culling & Dual-Pass Rendering
- **Frustum Culling**: `Frustum` extracts 6 view-projection planes (Gribb-Hartmann) and tests each chunk's AABB (`minP`, `maxP`) in `Renderer::drawWorld`, skipping chunks outside the camera's view.
- **Dual-Pass Rendering**:
  - **Pass 1 (Opaque)**: Draws `chunk->mesh` with depth writing and backface culling enabled.
  - **Pass 2 (Transparent / Cutout)**: Draws `chunk->transparentMesh` (e.g. oak leaves, wild tall grass diagonal cross-quads, water surfaces, and 3D torches) with two-sided rendering (`glDisable(GL_CULL_FACE)`) and shader alpha testing (`if (texel.a < 0.5) discard;`).

## Text rendering

`Font` builds a single-channel (`GL_R8`) atlas at startup from `FontData.hpp`,
a hand-authored 5x7 pixel font covering space, punctuation, digits and A-Z/a-z.
Each glyph occupies a 6x8 cell. `Renderer::drawText()` emits one quad per glyph,
mapping screen rows directly onto atlas rows (the atlas is authored top-down, so
text renders upright without any flip). `GL_NEAREST` keeps it crisp.

## Particles

`ParticleSystem` (`src/render/ParticleSystem.*`) owns a fixed-capacity pool of
256 particles in a single VBO, drawn in **one** batched call with the
`particle` shader. Torches emit two kinds: additive fire sparks and soft
rising smoke, both with a life decay so they flicker and fade. Quads are
camera-facing billboards, positioned in world space in the same frame as the
world draw, so they are depth-tested and fogged like everything else.

The pool is capped rather than grown, so a torch farm cannot degrade the frame
time; the cap is visible in the F3 overlay.

## Save format

`WorldSave` (`src/world/WorldSave.*`) writes `saves/<world name>.dat`:

- **Magic + version**: `0x53584F56` (`'VOXS'`) followed by a `uint32_t` version.
  `SAVE_VERSION_1` and `SAVE_VERSION_2` are both accepted on load; a file with
  any other version is rejected with a log line rather than crashing.
- **Payload**: world seed, chunk grid dimensions, player position/yaw/pitch and
  flight state, the 8-slot hotbar, the 24-slot storage inventory, the selected
  slot, and each chunk's blocks.
- **Compression**: chunk blocks are run-length encoded into `RLERun` runs, then
  the whole stream is RLE'd. v1 omitted the world name (it is recovered from the
  filename); v2 stores it in the header.
- **Directory helpers**: `WorldSave::listSavedWorlds()` enumerates `saves/*.dat`
  to populate save selector menus without loading whole voxel payloads.
  `WorldSave::deleteWorld(path)` unlinks the `.dat` file cleanly when deletion is confirmed.
- The world auto-saves on shutdown and on "Save and Quit to Title", and loads
  automatically at startup when a save exists.

## Menus & UI

`ui/Menu` is a small model: an ordered list of `Row`s that are either buttons
(`action`) or options (`value` + `adjust`). It handles selection, wrapping,
activation and hit-testing against the row rectangles cached during the last
`render()` call.

`Menu::render()` draws in a Legacy-console-inspired style: a dark beveled row
with a light top highlight, a gold outline + left marker for the selected row,
a title (or a large logo with shadow, underline and splash on the main menu)
and a footer with control hints. `Application` builds the main, pause and
options menus once in `buildMenus()`; option rows read and write the runtime
settings (`mouse sensitivity`, `FOV`, `view distance`, `VSync`, `fullscreen`,
`wireframe`) and call `applySettings()`.

`Application` picks the background for menu states: the main menu shows the
generated world through a slowly orbiting camera; the pause/options screens show
the frozen player camera under a translucent black dimmer.

### World selection & creation

`Select World`, `Create New World`, and `Delete World` are menus built dynamically in
`buildMenus()` using `WorldSave::listSavedWorlds()`. Selection lists the files in `saves/`
with their stored name and size; creation accepts a typed world name, a seed and one of the five world
types. Both are keyboard-navigable, and the name field is a text-entry row
rather than a button, so `Application` routes printable keys to it instead of
the menu's activation path. Deletion prompts with a secondary confirmation modal (`ConfirmDeleteWorld`).

### Sprite rendering

Hotbar and inventory slots need textured 16x16 block icons, which the flat `ui`
shader cannot draw. `Renderer::drawTexturedRect()` issues a quad through the
`sprite` shader against a per-slot sub-rectangle of the block atlas. Icons are
scaled by the GUI scale, so they track the HUD zoom.

## GUI scale

`Application::computeUiScale()` returns the current UI scale factor and pushes
it to `Renderer::setUIScale()` once per frame. Both the menus and the HUD read
`Renderer::uiScale()`, so they always match. The Options menu exposes
`GUI Scale` as `AUTO` or a fixed `1`-`4`; Auto is derived from the smaller
framebuffer dimension (`min(height, width * 0.6) / 280`, clamped to 1-8) so the
UI keeps a consistent apparent size instead of ballooning at high resolutions.

Menu layout reserves a bottom safe area (`footerBottom = max(22, height * 0.04)`)
so the footer text clears the Windows taskbar in fullscreen/borderless windows,
and clamps the button stack so the lowest row never overlaps the footer.

## Physics

`Player::update` builds a horizontal wish direction from the camera basis,
applies responsive voxel ground/air acceleration, gravity and jump, then moves **one axis at a time**.
`moveAxis` tests the destination AABB against solid blocks and, on a hit,
reverts that axis and zeroes its velocity. This gives stable wall-sliding
without a full sweep; the trade-off is a sub-millimetre gap when resting
against geometry.

`Player::collides` iterates the integer block range overlapping the AABB and
tests `isSolid()`.

### Responsive movement (air-strafing & bunnyhopping)

- **Ground acceleration & friction**: Ground movement uses snappy acceleration
  (`GROUND_ACCEL_RATE = 18.0f`) with crisp deceleration (`GROUND_DECEL_RATE = 20.0f`),
  providing immediate WASD steering in all directions (forward, backward, sideways, and diagonals).
- **Air control & mid-air strafing**: In-air movement provides responsive air steering
  (`AIR_ACCEL_RATE = 8.5f`), allowing players to easily steer sideways with A or D while jumping,
  jump around corners, and navigate parkour.
- **Bunnyhopping**: Jump inputs are queued (`JUMP_QUEUE_DURATION = 0.15s` or continuous hold).
  When touching ground with a pending jump, the player immediately jumps on the contact frame
  preserving accumulated horizontal momentum.

### Minecraft-style smooth view bobbing

- **Stable level horizon**: Camera pitch and roll are kept completely level (`m_roll = 0.0f`),
  preventing horizon rocking, screen tilt, and motion sickness/dizziness.
- **Continuous sinusoidal footstep sway**: Uses smooth trigonometric translation:
  - Vertical step dip: smooth cosine wave (`-(cos(timer) * 0.5 + 0.5) * 0.026m`), eliminating
    the non-differentiable shudder of raw `|sin(t)|`.
  - Horizontal weight sway: alternating lateral swing (`sin(0.5 * timer) * 0.022m`).
- **Air & water damping**: Bobbing intensity decays smoothly (`dt * 6.0`) when airborne or swimming,
  preventing abrupt pops when launching or landing.

### Sprinting & dynamic FOV

Holding `Left Ctrl` or double-tapping `W` sets `m_sprinting` (a timed latch, so
a brief double-tap is enough), which swaps `WALK_SPEED` for `SPRINT_SPEED`
(7.4 blocks/s) and drives a smooth FOV widening in `Player::update`. The FOV
interpolates toward its target rather than snapping, and the same path handles
the extra speed of creative flight. Flight itself is only available in creative
mode: the `F` toggle is ignored in survival, leaving creative force-disables
flight, and saves never restore a flying state (creative mode is not persisted).

### Water & swimming

`Player::update` samples the block at the feet and at mid-body; if either is
liquid (`isLiquid()` in `Block.hpp`) the player is in water. In water, gravity
is replaced by buoyancy, `Space` applies an active upward swim velocity, and
holding `Left Shift` dives. Left click raycasts pass through liquids rather than
targeting them, so you cannot mine water out of the world.

Underwater rendering is a separate concern: when the camera eye is submerged,
the `underwater` shader is applied as a full-screen filter with dense blue fog
and animated caustics. It triggers on the **eye** position, not the feet, so
wading chest-deep does not tint the screen.

## Profiling (F3 overlay)

`FrameStats` (`src/render/FrameStats.hpp`) is the single source of render
counters. `Renderer::beginFrame()` calls `resetFrame()` so the per-frame values
start at zero, then every draw path increments them as it issues work: chunk
meshes (via `Mesh::triangleCount()`), the entity batch, the particle batch, the
selection outline, and all 2D geometry -- `uploadUI` is the single funnel for UI
quads, so counting there covers rects, gradients and triangles at once.

`Application::run()` calls `accumulate(dt)` after `render()`, which folds the
finished frame into a fixed 90-frame rolling window. The overlay shows the
averages rather than the instantaneous values because per-frame numbers swing
wildly (a chunk rebuild or a menu redraw moves them by an order of magnitude).
The first 20 frames are skipped so world generation and first-frame shader
compiles do not poison the average.

`drawWorld` frustum-tests each chunk AABB once into a reusable buffer and shares
the result between the opaque and transparent passes. Testing per pass was both
wasteful and made the "culled" counter double-count.

`drawDebugOverlay` snapshots and restores the counters around its own drawing,
so the draw-call figure it displays does not include the overlay's own quads.
It uses its own scale (`m_uiScale * 0.75`, clamped) rather than the GUI scale, so
the readout stays legible even at GUI Scale 1.

Headless measurement: `maybeCapture()` logs a `Stats:` line next to every BMP
capture, and `VOXELMIN_CAPTURE_VSYNC=0` drops the vsync cap so frame time
reflects the engine rather than the display refresh rate.

Known gap: frame time is CPU-side only. OpenGL submission is asynchronous, so
GPU time is unmeasured. See the caveat in the `Current metrics` table in
`docs/PROGRESS.md`.

## Mobs & AI

`EntityManager` owns a flat list of `Mob`s and updates them in one pass; each
mob builds its geometry into a single shared vertex buffer, so all mobs cost one
draw call.

### Models & animation

A mob is a small set of axis-aligned cuboids (body, head, four legs, plus cow
horns) built from six quads each and textured from the pig/cow tiles in the
block atlas. Nothing is skinned: the walk cycle is a **swing angle** applied per
leg, driven by distance travelled, so legs stay in phase with ground speed and
stop when the mob stops. The head is a separate cuboid that can yaw toward the
player for the `AiLookAt` goal, and tilts with the mob's pitch.

`Renderer::drawEntities()` uploads the transformed geometry for every mob into
one batch, sampling skylight, blocklight and vertex AO at the mob's position, so
mobs darken in caves and near unlit torches exactly like static geometry.

`Mob` movement mirrors the player's axis-by-axis collision, but adds terrain
traversal on top. The order in `updatePhysics` matters:

1. Horizontal gait from the cached heading vector.
2. Vertical (`moveAxis(..., 1)`) resolves `m_onGround`.
3. Horizontal moves (`moveAxis(..., 0)` / `(..., 2)`) may then step up or jump,
   because they can trust `m_onGround`.

Resolving vertical **last** is what previously broke traversal: the step-up
branch tested `m_onGround` while it was always `false`, so mobs walked into
1-block ledges and stopped dead. A mob resting exactly on a surface can also
never generate a downward move to collide with, so an explicit 2 cm probe
confirms contact.

When a horizontal move is blocked and the mob is grounded:

- `tryJump` launches the mob upward (`m_velocity.y = kMobJumpVel`) to clear 1-block steps via authentic vertical acceleration and ballistic gravity arcs, rate-limited by `kJumpCooldown` (0.28s) to prevent jitter.
- `tryStepUp` remains in `AiGoalMask` (`AiStepUp`) as an optional capability, but is disabled by default for vanilla species because immediate vertical translation produces unnatural teleportation visuals.
- `updateStuckResponse` watches a per-mob `m_blockedTimer`: a short block triggers
  one hop, a sustained block triggers `rerollHeading()` so the mob picks a new
  direction instead of grinding against a wall.

`dropAhead` probes the column ~1.3 blocks ahead along the facing vector and
reports how far the ground drops; the `AiAvoidEdge` goal turns a wandering mob
away from any drop larger than `kSafeDrop` so it does not walk into caves.

### AI goals

Behaviours are named bits in an `AiGoalMask` (`src/entity/MobAi.hpp`) rather than
branches inside the update loop. Per-species tables (`kPigGoals`, `kCowGoals`, `kPigmanVillagerGoals`)
list which goals a species runs, so changing or disabling behaviour is a table
edit instead of a code change. Bits are ordered cheapest-test-first so the goals
that run most often exit after a single block probe. Heading trig is cached in
`updateForwardVector` and recomputed only when the yaw actually changes.

- `AiRetaliate`: When attacked by the player, switches from passive wander to aggressive melee pursuit at sprint speed with arms raised.
- `AiHomeLeash`: Tethers the mob to its village origin anchor. If aggro distance or displacement exceeds `kLeashDistance` (38m), the mob disengages from combat and returns home.

### Pigman Villagers & Village AI
- **Bipedal Geometry**: Features bipedal humanoid morphology with swinging arms and legs, a 3D snout box, and combat arm posing.
- **Group Defense System**: `EntityManager::alertNearbyPigmen(pos, radius)` allows any attacked pigman to alert all brethren within a 20-block radius to join the defense.
- **State Machine**: Cycles through `Idle`, `Wander`, `Hostile` (sprinting combat pursuit with attack damage), and `ReturnToVillage` (navigating back to home anchor).

Reference and rationale: `docs/SOURCES.md` section 4.

## Village Generation

`VillageGenerator` (`src/world/VillageGenerator.*`) creates procedural Pigman Settlements on the terrain surface:
- **Surface Elevation Snapping**: Uses `prepareFoundation` to probe the true surface height across building footprints and fills downward with solid cobblestone foundations, ensuring structures never float or spawn inside caves.
- **Multi-Template Layouts**: Generates one of four settlement topologies:
  1. `Crossroads`: 4-way intersecting main streets with residential cottages and crop fields.
  2. `LinearHighStreet`: A thoroughfare flanked by dwellings, farms, and lookout towers.
  3. `RingCommons`: Concentric market plaza surrounding a central water well.
  4. `FortifiedEnclave`: A walled hill settlement with central town hall and watchtower.
- **Building Archetypes**: Procedurally places water wells, wooden timber cottages with glass windows, watchtowers, irrigated farmlands with water canals, and lit torch posts.
- **Spawning & Tethering**: Places a village anchor recorded in `World::m_villages`, where `EntityManager::spawnInitialMobs` spawns resident Pigman Villagers tied to the village center.

## Interaction

`raycast()` is an Amanatides & Woo voxel DDA. It returns the first opaque block
and the face normal through which the ray entered. Left click sets that block to
`Air`; right click places the selected block at `block + normal`, unless that
cell would overlap the player. Right-clicking `Grass` or `Dirt` with a shovel
tills it into a `Dirt Path` (costs shovel durability in survival); a broken
`Dirt Path` drops `Dirt` via `getDropForBlock`.

## Loading Screen & World Generation

`Application::renderLoadingScreen` renders a Terraria-style animated loading screen during new world generation and loading:
- **Progress Tracking**: `TerrainGenerator` and `World` accept a `ProgressCallback` (`std::function<void(float, const std::string&)>`) that reports percentage completion ($0.0 \dots 1.0$) and phase descriptions ("Raising terrain & digging caves...", "Planting lush trees & foliage...", "Founding pigman settlements...", "Illuminating world & baking sunlight...").
- **Live UI & Event Pumping**: Renders a centered dark framed modal with gold border, world title, percentage readout, smooth progress bar, and rotating gameplay tips. Invokes `glfwSwapBuffers()` and `glfwPollEvents()` between stages to eliminate OS window lag/freeze warnings.

## Extension points

- **New block**: add to `BlockId`/`TextureTile`, the `defs[]` table, the atlas
  generator and `blockColor`. If the block has a non-cube shape, also add a `BlockBounds` entry and either an `emitCrossModel` path (foliage) or a custom mesh path (torch).
  - Directional blocks (like `WoodX`, `WoodZ` alongside `Wood`) define distinct block IDs mapping `WoodTop` end rings and `WoodSide` bark to corresponding faces in `ChunkMesher::getFaceTile()`, automatically resolved upon right-click block placement from the targeted face normal.
  - Pathway blocks (like `DirtPath`) define dedicated top and side soil textures for natural earthen village roadways.
- **Entity Hurt Flash**: Damage indicators use negative vertex AO (`ao = -1.0f`) in `Mob::buildGeometry()` when `m_hurtTimer > 0.0f`. `chunk.frag` detects `vAO < 0.0` and blends a vibrant red overlay (`vec3(1.0, 0.18, 0.18)`), replacing legacy black tinting.
- **New pre-generated world shape**: edit `TerrainGenerator`.
- **Infinite terrain**: replace the fixed chunk vector in `World` with a
  streaming map keyed on chunk coordinates; `getBlock`/`setBlock` already route
  through `chunkAt`.

#pragma once
#include <cstdint>

// ---------------------------------------------------------------------------
// Global compile-time configuration for VoxelMin.
// Keep every tunable "magic number" here so gameplay/world code stays clean.
// ---------------------------------------------------------------------------
namespace vox::config {

// Version scheme: v0.M<milestone>.<bump within milestone>. The milestone part
// ties the version to docs/ROADMAP.md, so "M5" and "v0.M5.x" can never drift.
// v0.1.0..v0.7.0 predate this scheme; see the version history in
// docs/PROGRESS.md for the legacy tags they replace.
inline constexpr const char* VERSION       = "0.M5.5";
inline constexpr int WINDOW_WIDTH          = 1280;
inline constexpr int WINDOW_HEIGHT         = 720;
inline constexpr const char* WINDOW_TITLE  = "VoxelMin v0.M5.5";
inline constexpr const char* SAVE_FILENAME = "world.dat";

// A chunk is a fixed-size column of blocks. This is the fundamental unit the
// world is split into for storage and mesh generation.
inline constexpr int CHUNK_SIZE_X = 16;
inline constexpr int CHUNK_SIZE_Y = 80;
inline constexpr int CHUNK_SIZE_Z = 16;

// The world is a fixed grid of chunks (32x32 chunks = 512x512 blocks).
inline constexpr int WORLD_CHUNKS_X = 32;
inline constexpr int WORLD_CHUNKS_Z = 32;

// Texture atlas layout: 16x16 tiles of 16x16 pixels = 256x256.
inline constexpr int ATLAS_TILES  = 16;
inline constexpr int TILE_PIXELS  = 16;
inline constexpr int ATLAS_PIXELS = ATLAS_TILES * TILE_PIXELS;

// World generation.
inline constexpr uint32_t WORLD_SEED = 1337u;

// Player / camera.
inline constexpr float PLAYER_EYE_HEIGHT = 1.62f;
inline constexpr float REACH_DISTANCE    = 6.0f;

// Rendering & Day/Night.
inline constexpr float FOG_START = 80.0f;
inline constexpr float FOG_END   = 160.0f;
inline constexpr float DAY_CYCLE_SECONDS = 600.0f; // 10 minutes per full 24h cycle

} // namespace vox::config

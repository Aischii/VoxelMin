#pragma once
#include <cstdint>

namespace vox {

// ---------------------------------------------------------------------------
// Per-frame render statistics.
//
// `Renderer` fills the per-frame counters as it issues work; `Application`
// feeds it the frame delta and the F3 overlay reads the result. Per-frame
// numbers are far too noisy to read directly (a chunk upload or a menu redraw
// swings them wildly), so the overlay shows a rolling average instead and the
// instantaneous values are kept for the capture log.
// ---------------------------------------------------------------------------
struct FrameStats {
    // --- instantaneous, reset at the start of every frame ---
    int drawCalls = 0;
    uint32_t triangles = 0;
    int chunksDrawn = 0;
    int chunksVisible = 0;
    int chunksCulled = 0;
    int uiDrawCalls = 0;

    // --- rolling averages ---
    float avgFrameMs = 0.0f;
    float avgFps = 0.0f;
    float avgDrawCalls = 0.0f;
    float avgTriangles = 0.0f;
    float avgChunksDrawn = 0.0f;
    float avgChunksVisible = 0.0f;
    float avgChunksCulled = 0.0f;
    // Clear the per-frame counters. Deliberately does NOT touch the warm-up
    // counter, which is driven by accumulate() and must survive across frames.
    void resetFrame();

    // Fold the finished frame into the averages. `dt` is the frame delta in
    // seconds.
    void accumulate(float dt);

private:
    // Frames folded in so far; averaging stops counting once the window is
    // full, so the cost is constant rather than growing.
    static constexpr int kWindow = 90;
    int m_frames = 0;
    int m_warmupFrames = 20;
    float m_sumFrameMs = 0.0f;
    float m_sumDrawCalls = 0.0f;
    float m_sumTriangles = 0.0f;
    float m_sumChunksDrawn = 0.0f;
    float m_sumChunksVisible = 0.0f;
    float m_sumChunksCulled = 0.0f;
};

} // namespace vox

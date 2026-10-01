#include "render/FrameStats.hpp"

namespace vox {

void FrameStats::resetFrame() {
    drawCalls = 0;
    triangles = 0;
    chunksDrawn = 0;
    chunksVisible = 0;
    chunksCulled = 0;
    uiDrawCalls = 0;
}

void FrameStats::accumulate(float dt) {
    const float frameMs = dt * 1000.0f;

    // Skip the warm-up frames, which include world generation and the first-frame
    // shader compiles and would otherwise poison the average for seconds.
    if (m_warmupFrames > 0) {
        --m_warmupFrames;
        return;
    }

    m_sumFrameMs += frameMs;
    m_sumDrawCalls += static_cast<float>(drawCalls);
    m_sumTriangles += static_cast<float>(triangles);
    m_sumChunksDrawn += static_cast<float>(chunksDrawn);
    m_sumChunksVisible += static_cast<float>(chunksVisible);
    m_sumChunksCulled += static_cast<float>(chunksCulled);

    if (m_frames < kWindow) {
        ++m_frames;
    } else {
        // Fixed-size window: keep the accumulated sum at ~kWindow samples by
        // shrinking it before folding in the new one, so the cost per frame is
        // constant instead of growing with uptime.
        const float shrink = static_cast<float>(kWindow) / static_cast<float>(kWindow + 1);
        m_sumFrameMs *= shrink;
        m_sumDrawCalls *= shrink;
        m_sumTriangles *= shrink;
        m_sumChunksDrawn *= shrink;
        m_sumChunksVisible *= shrink;
        m_sumChunksCulled *= shrink;
    }

    const float n = static_cast<float>(m_frames);
    avgFrameMs = m_sumFrameMs / n;
    avgDrawCalls = m_sumDrawCalls / n;
    avgTriangles = m_sumTriangles / n;
    avgChunksDrawn = m_sumChunksDrawn / n;
    avgChunksVisible = m_sumChunksVisible / n;
    avgChunksCulled = m_sumChunksCulled / n;
    // Derived from the smoothed frame time, not from the instantaneous delta, so
    // the two numbers on the overlay always agree with each other.
    avgFps = (avgFrameMs > 0.0f) ? (1000.0f / avgFrameMs) : 0.0f;
}

} // namespace vox

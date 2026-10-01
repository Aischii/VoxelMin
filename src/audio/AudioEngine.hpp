#pragma once
#include <glm/glm.hpp>
#include <mutex>
#include <vector>
#include <cstddef>

namespace vox {

enum class SoundId {
    None = 0,
    Click,
    ItemPickup,
    StepGrass,
    StepStone,
    StepWood,
    DigGrass,
    DigStone,
    DigWood,
    PlaceBlock,
    MobHurt,
    ToolBreak,
    Count
};

// Procedural audio engine powered by miniaudio.
// Zero external sound asset dependencies: all sound effects and ambient wind
// are procedurally synthesized at startup and mixed with spatial 3D panning.
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool init();
    void shutdown();
    bool isAvailable() const { return m_initialized; }

    // Play one-shot 2D sound effect (UI, local events)
    void play(SoundId id, float volume = 1.0f, float pitch = 1.0f);

    // Play spatialized 3D sound effect with distance attenuation & stereo panning
    void play3D(SoundId id, const glm::vec3& worldPos, const glm::vec3& listenerPos,
                const glm::vec3& listenerFront, float volume = 1.0f);

    // Volume adjustments (0.0 to 1.0)
    void setMasterVolume(float v);
    float masterVolume() const { return m_masterVolume; }

    void setSfxVolume(float v);
    float sfxVolume() const { return m_sfxVolume; }

    void setAmbientVolume(float v);
    float ambientVolume() const { return m_ambientVolume; }

    void setInGame(bool inGame) { m_inGame = inGame; }

    // High-priority audio mixing callback
    void mixAudio(float* output, size_t frameCount, size_t channels);

private:
    struct Voice {
        SoundId id = SoundId::None;
        float samplePos = 0.0f;
        float pitch = 1.0f;
        float volumeLeft = 1.0f;
        float volumeRight = 1.0f;
        bool active = false;
    };

    struct SoundSample {
        std::vector<float> data;
    };

    void precomputeSounds();

    bool m_initialized = false;
    bool m_inGame = false;
    float m_masterVolume = 0.8f;
    float m_sfxVolume = 0.8f;
    float m_ambientVolume = 0.35f;

    std::vector<SoundSample> m_samples;
    std::vector<float> m_windLoop;
    float m_windPos = 0.0f;

    static constexpr size_t MAX_VOICES = 32;
    Voice m_voices[MAX_VOICES];
    std::mutex m_voiceMutex;

    void* m_device = nullptr; // Opaque pointer to ma_device
};

} // namespace vox

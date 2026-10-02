#pragma once
#include <glm/glm.hpp>
#include <mutex>
#include <string>
#include <vector>
#include <cstddef>
#include <algorithm>

namespace vox {

class World;

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
    PlayerHurt,
    PlayerEat,
    PlayerBurp,
    WaterSplash,
    WaterFlow,
    StepDampWool,
    PhantomFootstep,
    DistantClock,
    Count
};

// ---------------------------------------------------------------------------
// 1-Pole Low-Pass Filter for acoustic occlusion and underwater muffling.
// ---------------------------------------------------------------------------
struct LowPassFilter {
    float y1 = 0.0f;
    float alpha = 1.0f; // 1.0 = bypass

    void setCutoff(float cutoffHz, float sampleRate = 44100.0f) {
        cutoffHz = std::clamp(cutoffHz, 100.0f, 20000.0f);
        const float dt = 1.0f / sampleRate;
        const float rc = 1.0f / (2.0f * 3.14159265358979323846f * cutoffHz);
        alpha = dt / (rc + dt);
    }

    void reset() {
        y1 = 0.0f;
    }

    float process(float in) {
        y1 += alpha * (in - y1);
        return y1;
    }
};

// ---------------------------------------------------------------------------
// Freeverb / Schroeder DSP Reverberation Components
// ---------------------------------------------------------------------------
struct CombFilter {
    std::vector<float> buffer;
    size_t bufIdx = 0;
    float filterStore = 0.0f;
    float feedback = 0.8f;
    float damp = 0.2f;

    void init(size_t size) {
        buffer.assign(size, 0.0f);
        bufIdx = 0;
        filterStore = 0.0f;
    }

    float process(float input) {
        if (buffer.empty()) return input;
        float output = buffer[bufIdx];
        filterStore = (output * (1.0f - damp)) + (filterStore * damp);
        buffer[bufIdx] = input + (filterStore * feedback);
        if (++bufIdx >= buffer.size()) bufIdx = 0;
        return output;
    }
};

struct AllPassFilter {
    std::vector<float> buffer;
    size_t bufIdx = 0;
    float feedback = 0.5f;

    void init(size_t size) {
        buffer.assign(size, 0.0f);
        bufIdx = 0;
    }

    float process(float input) {
        if (buffer.empty()) return input;
        float bufOut = buffer[bufIdx];
        float output = -input + bufOut;
        buffer[bufIdx] = input + (bufOut * feedback);
        if (++bufIdx >= buffer.size()) bufIdx = 0;
        return output;
    }
};

// ---------------------------------------------------------------------------
// Procedural audio engine powered by miniaudio with Sound Physics Remastered:
// - Ray-traced acoustic occlusion (DDA through solid blocks)
// - Dynamic cave/room reverberation & Schroeder feedback delay networks
// - Material-specific high-frequency absorption
// - Underwater acoustic modeling & bubbling ambient soundscape
// ---------------------------------------------------------------------------
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool init();
    void shutdown();
    bool isAvailable() const { return m_initialized; }

    // Play one-shot 2D sound effect (UI, local events)
    void play(SoundId id, float volume = 1.0f, float pitch = 1.0f);

    // Play spatialized 3D sound effect with ray-traced occlusion and distance attenuation
    void play3D(SoundId id, const glm::vec3& worldPos, const glm::vec3& listenerPos,
                const glm::vec3& listenerFront, float volume = 1.0f, const World* world = nullptr);

    // Environmental sound physics update (room size, cave echo probe, underwater state, night/day factor)
    void updateEnvironment(const World& world, const glm::vec3& listenerPos, bool isUnderwater, float nightFactor = 0.0f);

    // Volume adjustments (0.0 to 1.0)
    void setMasterVolume(float v);
    float masterVolume() const { return m_masterVolume; }

    void setSfxVolume(float v);
    float sfxVolume() const { return m_sfxVolume; }

    void setAmbientVolume(float v);
    float ambientVolume() const { return m_ambientVolume; }

    void setMusicVolume(float v);
    float musicVolume() const { return m_musicVolume; }

    bool loadMusicFile(const std::string& path);
    bool loadMenuMusicFile(const std::string& path);
    bool loadGameMusicFile(const std::string& path);
    void scanAndLoadMusic();

    void setInGame(bool inGame) { m_inGame = inGame; }
    void setInBackrooms(bool inBackrooms) { m_inBackrooms = inBackrooms; }
    bool isUnderwater() const { return m_isUnderwater; }

    // High-priority audio mixing callback
    void mixAudio(float* output, size_t frameCount, size_t channels);

private:
    struct Voice {
        SoundId id = SoundId::None;
        float samplePos = 0.0f;
        float pitch = 1.0f;
        float volumeLeft = 1.0f;
        float volumeRight = 1.0f;
        LowPassFilter lpfLeft;
        LowPassFilter lpfRight;
        bool isOccluded = false;
        bool active = false;
    };

    struct SoundSample {
        std::vector<float> data;
    };

    void precomputeSounds();
    void synthesizeMenuMusic();
    void synthesizeGameMusic();
    void synthesizeBackroomsMusic();
    void initReverb();

    bool m_initialized = false;
    bool m_inGame = false;
    bool m_inBackrooms = false;
    bool m_isUnderwater = false;
    float m_masterVolume = 0.8f;
    float m_sfxVolume = 0.8f;
    float m_ambientVolume = 0.35f;
    float m_musicVolume = 0.5f;

    std::vector<SoundSample> m_samples;
    std::vector<float> m_windLoop;
    float m_windPos = 0.0f;

    std::vector<float> m_cricketLoop;
    float m_cricketPos = 0.0f;

    std::vector<float> m_caveDroneLoop;
    float m_caveDronePos = 0.0f;

    std::vector<float> m_underwaterLoop;
    float m_underwaterPos = 0.0f;

    float m_nightFactor = 0.0f;
    float m_caveFactor = 0.0f;

    // Background music stereo PCM buffers (interleaved L, R)
    std::vector<float> m_menuMusicPcm;
    float m_menuMusicPos = 0.0f;
    bool m_menuMusicLoaded = false;

    std::vector<float> m_gameMusicPcm;
    float m_gameMusicPos = 0.0f;
    bool m_gameMusicLoaded = false;

    std::vector<float> m_backroomsMusicPcm;
    float m_backroomsMusicPos = 0.0f;
    bool m_backroomsMusicLoaded = false;

    float m_menuGain = 1.0f;
    float m_gameGain = 0.0f;
    float m_backroomsGain = 0.0f;
    std::mutex m_musicMutex;

    // Environmental Reverberation DSP
    static constexpr size_t NUM_COMBS = 4;
    static constexpr size_t NUM_ALLPASS = 2;
    CombFilter m_combsL[NUM_COMBS];
    CombFilter m_combsR[NUM_COMBS];
    AllPassFilter m_allPassL[NUM_ALLPASS];
    AllPassFilter m_allPassR[NUM_ALLPASS];

    float m_roomSize = 0.2f;
    float m_reverbDamp = 0.3f;
    float m_wetMix = 0.05f;
    float m_targetRoomSize = 0.2f;
    float m_targetDamp = 0.3f;
    float m_targetWetMix = 0.05f;
    LowPassFilter m_underwaterFilterL;
    LowPassFilter m_underwaterFilterR;
    std::mutex m_envMutex;

    static constexpr size_t MAX_VOICES = 32;
    Voice m_voices[MAX_VOICES];
    std::mutex m_voiceMutex;

    void* m_device = nullptr; // Opaque pointer to ma_device
};

} // namespace vox

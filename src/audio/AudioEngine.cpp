#include "audio/AudioEngine.hpp"
#include "core/Log.hpp"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#include "miniaudio.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <cmath>
#include <random>
#include <algorithm>

namespace vox {

namespace {

constexpr float PI = 3.14159265358979323846f;
constexpr int SAMPLE_RATE = 44100;

void audioDeviceCallback(ma_device* pDevice, void* pOutput, const void* /*pInput*/, ma_uint32 frameCount) {
    auto* engine = static_cast<AudioEngine*>(pDevice->pUserData);
    if (engine) {
        engine->mixAudio(static_cast<float*>(pOutput), static_cast<size_t>(frameCount), 2);
    }
}

} // namespace

AudioEngine::AudioEngine() {
    for (size_t i = 0; i < MAX_VOICES; ++i) {
        m_voices[i].active = false;
    }
}

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::init() {
    if (m_initialized) return true;

    precomputeSounds();

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate        = SAMPLE_RATE;
    config.dataCallback      = audioDeviceCallback;
    config.pUserData         = this;

    auto* dev = new ma_device();
    m_device = dev;

    if (ma_device_init(nullptr, &config, dev) != MA_SUCCESS) {
        log::warn("Could not initialize audio device; running in silent mode");
        delete dev;
        m_device = nullptr;
        m_initialized = false;
        return false;
    }

    if (ma_device_start(dev) != MA_SUCCESS) {
        log::warn("Could not start audio device playback; running in silent mode");
        ma_device_uninit(dev);
        delete dev;
        m_device = nullptr;
        m_initialized = false;
        return false;
    }

    m_initialized = true;
    log::info("Audio engine initialised (miniaudio 44.1kHz stereo, %d procedural sound banks)",
              static_cast<int>(m_samples.size()));
    return true;
}

void AudioEngine::shutdown() {
    if (m_device) {
        auto* dev = static_cast<ma_device*>(m_device);
        ma_device_stop(dev);
        ma_device_uninit(dev);
        delete dev;
        m_device = nullptr;
    }
    m_initialized = false;
}

void AudioEngine::setMasterVolume(float v) {
    m_masterVolume = std::clamp(v, 0.0f, 1.0f);
}

void AudioEngine::setSfxVolume(float v) {
    m_sfxVolume = std::clamp(v, 0.0f, 1.0f);
}

void AudioEngine::setAmbientVolume(float v) {
    m_ambientVolume = std::clamp(v, 0.0f, 1.0f);
}

void AudioEngine::play(SoundId id, float volume, float pitch) {
    if (!m_initialized || id == SoundId::None || m_masterVolume <= 0.001f || m_sfxVolume <= 0.001f) return;

    static std::mt19937 pitchRng(456);
    std::uniform_real_distribution<float> pDist(0.96f, 1.04f);
    const float finalPitch = std::clamp(pitch * pDist(pitchRng), 0.5f, 2.0f);

    std::lock_guard<std::mutex> lock(m_voiceMutex);
    for (size_t v = 0; v < MAX_VOICES; ++v) {
        if (!m_voices[v].active) {
            m_voices[v].id = id;
            m_voices[v].samplePos = 0.0f;
            m_voices[v].pitch = finalPitch;
            m_voices[v].volumeLeft = volume;
            m_voices[v].volumeRight = volume;
            m_voices[v].active = true;
            return;
        }
    }
}

void AudioEngine::play3D(SoundId id, const glm::vec3& worldPos, const glm::vec3& listenerPos,
                        const glm::vec3& listenerFront, float volume) {
    if (!m_initialized || id == SoundId::None || m_masterVolume <= 0.001f || m_sfxVolume <= 0.001f) return;

    const glm::vec3 diff = worldPos - listenerPos;
    const float dist = glm::length(diff);
    if (dist > 30.0f) return;

    // Distance attenuation
    const float atten = 1.0f / (1.0f + 0.14f * dist * dist);
    const float finalVol = volume * atten;
    if (finalVol < 0.005f) return;

    // Stereo panning based on listener right vector
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    glm::vec3 listenerRight = glm::cross(listenerFront, worldUp);
    if (glm::length(listenerRight) > 1e-4f) {
        listenerRight = glm::normalize(listenerRight);
    } else {
        listenerRight = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    float pan = 0.0f;
    if (dist > 0.05f) {
        const glm::vec3 dir = diff / dist;
        pan = std::clamp(glm::dot(dir, listenerRight), -1.0f, 1.0f);
    }

    const float volL = finalVol * std::clamp(1.0f - pan * 0.75f, 0.0f, 1.0f);
    const float volR = finalVol * std::clamp(1.0f + pan * 0.75f, 0.0f, 1.0f);

    static std::mt19937 pitchRng(123);
    std::uniform_real_distribution<float> pDist(0.94f, 1.06f);
    const float finalPitch = std::clamp(pDist(pitchRng), 0.5f, 2.0f);

    std::lock_guard<std::mutex> lock(m_voiceMutex);
    for (size_t v = 0; v < MAX_VOICES; ++v) {
        if (!m_voices[v].active) {
            m_voices[v].id = id;
            m_voices[v].samplePos = 0.0f;
            m_voices[v].pitch = finalPitch;
            m_voices[v].volumeLeft = volL;
            m_voices[v].volumeRight = volR;
            m_voices[v].active = true;
            return;
        }
    }
}

void AudioEngine::mixAudio(float* output, size_t frameCount, size_t channels) {
    std::fill(output, output + frameCount * channels, 0.0f);
    if (!m_initialized || m_masterVolume <= 0.001f) return;

    // 1. Seamless ambient wind
    if (m_inGame && m_ambientVolume > 0.001f && !m_windLoop.empty()) {
        const float windVol = m_masterVolume * m_ambientVolume;
        const size_t windLen = m_windLoop.size();
        for (size_t f = 0; f < frameCount; ++f) {
            size_t idx = static_cast<size_t>(m_windPos);
            float sample = m_windLoop[idx % windLen] * windVol;
            m_windPos += 1.0f;
            if (m_windPos >= static_cast<float>(windLen)) {
                m_windPos -= static_cast<float>(windLen);
            }

            output[f * channels + 0] += sample;
            if (channels > 1) {
                output[f * channels + 1] += sample;
            }
        }
    }

    // 2. Active polyphonic SFX voices
    if (m_sfxVolume > 0.001f) {
        const float sfxMaster = m_masterVolume * m_sfxVolume;
        std::lock_guard<std::mutex> lock(m_voiceMutex);
        for (size_t v = 0; v < MAX_VOICES; ++v) {
            Voice& voice = m_voices[v];
            if (!voice.active) continue;

            const size_t sampleIdx = static_cast<size_t>(voice.id);
            if (sampleIdx >= m_samples.size()) {
                voice.active = false;
                continue;
            }

            const auto& sampleData = m_samples[sampleIdx].data;
            const size_t totalSamples = sampleData.size();

            for (size_t f = 0; f < frameCount; ++f) {
                size_t idx = static_cast<size_t>(voice.samplePos);
                if (idx >= totalSamples) {
                    voice.active = false;
                    break;
                }

                const float sample = sampleData[idx];
                output[f * channels + 0] += sample * voice.volumeLeft * sfxMaster;
                if (channels > 1) {
                    output[f * channels + 1] += sample * voice.volumeRight * sfxMaster;
                }

                voice.samplePos += voice.pitch;
            }
        }
    }

    // 3. Soft limiter to prevent clipping
    for (size_t i = 0; i < frameCount * channels; ++i) {
        output[i] = std::clamp(output[i], -1.0f, 1.0f);
    }
}

void AudioEngine::precomputeSounds() {
    m_samples.resize(static_cast<size_t>(SoundId::Count));

    // SoundId::Click (Crisp UI button click)
    {
        const int n = static_cast<int>(0.022f * SAMPLE_RATE);
        std::vector<float> data(n);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 220.0f);
            const float wave = 0.65f * std::sin(2.0f * PI * 1400.0f * t)
                             + 0.35f * std::sin(2.0f * PI * 700.0f * t);
            data[i] = wave * env * 0.45f;
        }
        m_samples[static_cast<size_t>(SoundId::Click)].data = std::move(data);
    }

    // SoundId::ItemPickup (Cheerful upward chime/pop)
    {
        const int n = static_cast<int>(0.13f * SAMPLE_RATE);
        std::vector<float> data(n);
        float phase = 0.0f;
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float norm = t / 0.13f;
            const float freq = 620.0f + 640.0f * norm;
            phase += (2.0f * PI * freq) / SAMPLE_RATE;
            const float env = (norm < 0.06f) ? (norm / 0.06f) : std::exp(-(norm - 0.06f) * 7.0f);
            const float wave = 0.72f * std::sin(phase) + 0.28f * std::sin(phase * 2.0f);
            data[i] = wave * env * 0.45f;
        }
        m_samples[static_cast<size_t>(SoundId::ItemPickup)].data = std::move(data);
    }

    // SoundId::StepGrass (Rustling grass step)
    {
        const int n = static_cast<int>(0.09f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        float lp = 0.0f;
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::pow(t / 0.09f, 0.4f) * std::exp(-t * 55.0f) * 2.5f;
            const float noise = dist(rng);
            lp += (noise - lp) * 0.28f;
            const float thud = 0.32f * std::sin(2.0f * PI * 95.0f * t) * std::exp(-t * 40.0f);
            data[i] = (lp * 0.68f + thud) * env * 0.38f;
        }
        m_samples[static_cast<size_t>(SoundId::StepGrass)].data = std::move(data);
    }

    // SoundId::StepStone (Hard stone footstep)
    {
        const int n = static_cast<int>(0.08f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(1337);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        float lp = 0.0f;
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 65.0f);
            const float noise = dist(rng);
            lp += (noise - lp) * 0.5f;
            const float click = 0.42f * std::sin(2.0f * PI * 1800.0f * t) * std::exp(-t * 200.0f);
            const float thud = 0.35f * std::sin(2.0f * PI * 160.0f * t) * std::exp(-t * 50.0f);
            data[i] = (lp * 0.35f + click + thud) * env * 0.38f;
        }
        m_samples[static_cast<size_t>(SoundId::StepStone)].data = std::move(data);
    }

    // SoundId::StepWood (Hollow wooden step)
    {
        const int n = static_cast<int>(0.085f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(555);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float freq = 290.0f - 110.0f * (t / 0.085f);
            const float env = std::exp(-t * 45.0f);
            const float wave = std::sin(2.0f * PI * freq * t);
            const float click = dist(rng) * 0.22f * std::exp(-t * 120.0f);
            data[i] = (wave * 0.65f + click) * env * 0.42f;
        }
        m_samples[static_cast<size_t>(SoundId::StepWood)].data = std::move(data);
    }

    // SoundId::DigGrass (Crunchy grass break)
    {
        const int n = static_cast<int>(0.15f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(999);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        float filter = 0.0f;
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::pow(t / 0.15f, 0.3f) * std::exp(-t * 28.0f) * 2.2f;
            const float noise = dist(rng);
            filter += (noise - filter) * 0.36f;
            const float pop = 0.32f * std::sin(2.0f * PI * 110.0f * t) * std::exp(-t * 30.0f);
            data[i] = (filter * 0.8f + pop) * env * 0.45f;
        }
        m_samples[static_cast<size_t>(SoundId::DigGrass)].data = std::move(data);
    }

    // SoundId::DigStone (Rock crack / shatter)
    {
        const int n = static_cast<int>(0.18f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(777);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        float filter = 0.0f;
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 22.0f);
            const float noise = dist(rng);
            filter += (noise - filter) * 0.48f;
            const float thud = 0.42f * std::sin(2.0f * PI * 85.0f * t) * std::exp(-t * 25.0f);
            const float clack = 0.32f * std::sin(2.0f * PI * 820.0f * t) * std::exp(-t * 70.0f);
            data[i] = (filter * 0.52f + thud + clack) * env * 0.50f;
        }
        m_samples[static_cast<size_t>(SoundId::DigStone)].data = std::move(data);
    }

    // SoundId::DigWood (Wood splintering snap)
    {
        const int n = static_cast<int>(0.16f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(4321);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 26.0f);
            const float freq = 340.0f - 180.0f * (t / 0.16f);
            const float wave = std::sin(2.0f * PI * freq * t);
            const float crack = dist(rng) * 0.42f * std::exp(-t * 40.0f);
            data[i] = (wave * 0.52f + crack) * env * 0.46f;
        }
        m_samples[static_cast<size_t>(SoundId::DigWood)].data = std::move(data);
    }

    // SoundId::PlaceBlock (Solid placement clack)
    {
        const int n = static_cast<int>(0.11f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(2024);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 38.0f);
            const float thud = std::sin(2.0f * PI * 140.0f * t);
            const float snap = dist(rng) * 0.38f * std::exp(-t * 110.0f);
            data[i] = (thud * 0.62f + snap) * env * 0.44f;
        }
        m_samples[static_cast<size_t>(SoundId::PlaceBlock)].data = std::move(data);
    }

    // SoundId::MobHurt (Fleshy punch / grunt)
    {
        const int n = static_cast<int>(0.18f * SAMPLE_RATE);
        std::vector<float> data(n);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 18.0f);
            const float freq = 150.0f - 80.0f * (t / 0.18f);
            const float wave = std::sin(2.0f * PI * freq * t);
            const float distorted = std::tanh(wave * 2.2f) * 0.52f;
            data[i] = distorted * env * 0.55f;
        }
        m_samples[static_cast<size_t>(SoundId::MobHurt)].data = std::move(data);
    }

    // SoundId::ToolBreak (Metallic snap & fracture)
    {
        const int n = static_cast<int>(0.30f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(8888);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 14.0f);
            const float chime = 0.42f * std::sin(2.0f * PI * 1900.0f * t)
                              + 0.28f * std::sin(2.0f * PI * 2700.0f * t);
            const float crunch = dist(rng) * 0.42f * std::exp(-t * 22.0f);
            data[i] = (chime + crunch) * env * 0.50f;
        }
        m_samples[static_cast<size_t>(SoundId::ToolBreak)].data = std::move(data);
    }

    // Seamless ambient wind loop (~4 seconds)
    {
        const int n = 4 * SAMPLE_RATE;
        m_windLoop.resize(n);
        std::mt19937 rng(12345);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
        for (int i = 0; i < n; ++i) {
            const float white = dist(rng);
            b0 = 0.99765f * b0 + white * 0.0990460f;
            b1 = 0.96300f * b1 + white * 0.2965164f;
            b2 = 0.57000f * b2 + white * 1.0526913f;
            float pink = (b0 + b1 + b2 + white * 0.1848f) * 0.05f;

            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float swell = 0.72f + 0.28f * std::sin(2.0f * PI * 0.25f * t);
            m_windLoop[i] = pink * swell * 0.14f;
        }
        const int crossfade = 2500;
        for (int i = 0; i < crossfade; ++i) {
            const float alpha = static_cast<float>(i) / crossfade;
            m_windLoop[n - crossfade + i] = glm::mix(m_windLoop[n - crossfade + i], m_windLoop[i], alpha);
        }
    }
}

} // namespace vox

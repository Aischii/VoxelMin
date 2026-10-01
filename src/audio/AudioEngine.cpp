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
#include <filesystem>

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
    scanAndLoadMusic();

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
    log::info("Audio engine initialised (miniaudio 44.1kHz stereo, %d procedural sound banks, BGM active)",
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

void AudioEngine::setMusicVolume(float v) {
    m_musicVolume = std::clamp(v, 0.0f, 1.0f);
}

bool AudioEngine::loadMusicFile(const std::string& path) {
    return loadGameMusicFile(path) && loadMenuMusicFile(path);
}

bool AudioEngine::loadMenuMusicFile(const std::string& path) {
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, SAMPLE_RATE);
    ma_decoder decoder;
    if (ma_decoder_init_file(path.c_str(), &decoderConfig, &decoder) != MA_SUCCESS) {
        return false;
    }

    std::vector<float> pcmData;
    float tempBuf[4096 * 2];
    ma_uint64 framesRead = 0;
    while (ma_decoder_read_pcm_frames(&decoder, tempBuf, 4096, &framesRead) == MA_SUCCESS && framesRead > 0) {
        pcmData.insert(pcmData.end(), tempBuf, tempBuf + framesRead * 2);
    }
    ma_decoder_uninit(&decoder);

    if (pcmData.empty()) return false;

    {
        std::lock_guard<std::mutex> lock(m_musicMutex);
        m_menuMusicPcm = std::move(pcmData);
        m_menuMusicPos = 0.0f;
        m_menuMusicLoaded = true;
    }
    log::info("Loaded menu background music track: %s (%.1f seconds)",
              path.c_str(), static_cast<float>(m_menuMusicPcm.size() / 2) / static_cast<float>(SAMPLE_RATE));
    return true;
}

bool AudioEngine::loadGameMusicFile(const std::string& path) {
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, SAMPLE_RATE);
    ma_decoder decoder;
    if (ma_decoder_init_file(path.c_str(), &decoderConfig, &decoder) != MA_SUCCESS) {
        return false;
    }

    std::vector<float> pcmData;
    float tempBuf[4096 * 2];
    ma_uint64 framesRead = 0;
    while (ma_decoder_read_pcm_frames(&decoder, tempBuf, 4096, &framesRead) == MA_SUCCESS && framesRead > 0) {
        pcmData.insert(pcmData.end(), tempBuf, tempBuf + framesRead * 2);
    }
    ma_decoder_uninit(&decoder);

    if (pcmData.empty()) return false;

    {
        std::lock_guard<std::mutex> lock(m_musicMutex);
        m_gameMusicPcm = std::move(pcmData);
        m_gameMusicPos = 0.0f;
        m_gameMusicLoaded = true;
    }
    log::info("Loaded in-game background music track: %s (%.1f seconds)",
              path.c_str(), static_cast<float>(m_gameMusicPcm.size() / 2) / static_cast<float>(SAMPLE_RATE));
    return true;
}

void AudioEngine::scanAndLoadMusic() {
    const std::vector<std::string> searchDirs = {
        "assets/music", "assets/audio",
        "../assets/music", "../assets/audio",
        "../../assets/music", "../../assets/audio"
    };

    bool menuFound = false;
    bool gameFound = false;
    std::string fallbackTrack;

    std::error_code ec;
    for (const auto& dir : searchDirs) {
        if (!std::filesystem::exists(dir, ec)) continue;
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (!entry.is_regular_file()) continue;
            const auto ext = entry.path().extension().string();
            if (ext == ".mp3" || ext == ".wav" || ext == ".flac" || ext == ".ogg") {
                const std::string filename = entry.path().stem().string();
                const std::string fullPath = entry.path().string();

                if (fallbackTrack.empty()) fallbackTrack = fullPath;

                if (filename.find("menu") != std::string::npos || filename.find("title") != std::string::npos) {
                    if (loadMenuMusicFile(fullPath)) menuFound = true;
                } else if (filename.find("game") != std::string::npos || filename.find("ambient") != std::string::npos ||
                           filename.find("bgm") != std::string::npos) {
                    if (loadGameMusicFile(fullPath)) gameFound = true;
                }
            }
        }
    }

    if (!menuFound && !fallbackTrack.empty() && gameFound) {
        loadMenuMusicFile(fallbackTrack);
        menuFound = true;
    }
    if (!gameFound && !fallbackTrack.empty() && menuFound) {
        loadGameMusicFile(fallbackTrack);
        gameFound = true;
    }

    if (!menuFound) {
        synthesizeMenuMusic();
    }
    if (!gameFound) {
        synthesizeGameMusic();
    }
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

    // 1. Background music (BGM) with smooth crossfade between Menu and In-Game
    if (m_musicVolume > 0.001f) {
        const float musicVol = m_masterVolume * m_musicVolume * 0.45f;
        const float targetMenuGain = m_inGame ? 0.0f : 1.0f;
        const float targetGameGain = m_inGame ? 1.0f : 0.0f;
        const float gainSlew = 1.0f / (1.0f * SAMPLE_RATE); // 1.0 second crossfade

        std::lock_guard<std::mutex> lock(m_musicMutex);

        const size_t menuLen = m_menuMusicPcm.size() / 2;
        const size_t gameLen = m_gameMusicPcm.size() / 2;

        for (size_t f = 0; f < frameCount; ++f) {
            // Slew gains
            if (m_menuGain < targetMenuGain) {
                m_menuGain = std::min(targetMenuGain, m_menuGain + gainSlew);
            } else if (m_menuGain > targetMenuGain) {
                m_menuGain = std::max(targetMenuGain, m_menuGain - gainSlew);
            }

            if (m_gameGain < targetGameGain) {
                m_gameGain = std::min(targetGameGain, m_gameGain + gainSlew);
            } else if (m_gameGain > targetGameGain) {
                m_gameGain = std::max(targetGameGain, m_gameGain - gainSlew);
            }

            float left = 0.0f;
            float right = 0.0f;

            // Menu Music stream
            if (m_menuGain > 0.001f && menuLen > 0) {
                const size_t frameIdx = static_cast<size_t>(m_menuMusicPos) % menuLen;
                const float mVol = musicVol * m_menuGain;
                left += m_menuMusicPcm[frameIdx * 2 + 0] * mVol;
                right += m_menuMusicPcm[frameIdx * 2 + 1] * mVol;

                m_menuMusicPos += 1.0f;
                if (m_menuMusicPos >= static_cast<float>(menuLen)) {
                    m_menuMusicPos -= static_cast<float>(menuLen);
                }
            }

            // Game Music stream
            if (m_gameGain > 0.001f && gameLen > 0) {
                const size_t frameIdx = static_cast<size_t>(m_gameMusicPos) % gameLen;
                const float gVol = musicVol * m_gameGain;
                left += m_gameMusicPcm[frameIdx * 2 + 0] * gVol;
                right += m_gameMusicPcm[frameIdx * 2 + 1] * gVol;

                m_gameMusicPos += 1.0f;
                if (m_gameMusicPos >= static_cast<float>(gameLen)) {
                    m_gameMusicPos -= static_cast<float>(gameLen);
                }
            }

            output[f * channels + 0] += left;
            if (channels > 1) {
                output[f * channels + 1] += right;
            }
        }
    }

    // 2. Seamless ambient wind
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

    // 3. Active polyphonic SFX voices
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

    // 4. Soft limiter to prevent clipping
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

    // SoundId::PlayerHurt (Visceral punch impact + low groan/thud)
    {
        const int n = static_cast<int>(0.22f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(999);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 16.0f);
            const float freq = 120.0f - 60.0f * (t / 0.22f);
            const float thud = std::sin(2.0f * PI * freq * t);
            const float crunch = dist(rng) * 0.35f * std::exp(-t * 30.0f);
            data[i] = (thud * 0.75f + crunch) * env * 0.65f;
        }
        m_samples[static_cast<size_t>(SoundId::PlayerHurt)].data = std::move(data);
    }

    // SoundId::PlayerEat (Crisp, crunchy bite/chew)
    {
        const int n = static_cast<int>(0.12f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(5555);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 26.0f);
            const float noise = dist(rng);
            const float tonal = 0.3f * std::sin(2.0f * PI * 850.0f * t) + 0.2f * std::sin(2.0f * PI * 1300.0f * t);
            data[i] = (noise * 0.7f + tonal) * env * 0.40f;
        }
        m_samples[static_cast<size_t>(SoundId::PlayerEat)].data = std::move(data);
    }

    // SoundId::PlayerBurp (Low resonant burp after full meal)
    {
        const int n = static_cast<int>(0.35f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(777);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::sin(PI * (t / 0.35f)) * std::exp(-t * 3.5f);
            const float freq = 85.0f + 15.0f * std::sin(2.0f * PI * 12.0f * t);
            const float pulse = std::sin(2.0f * PI * freq * t);
            const float rattle = dist(rng) * 0.25f * (0.5f + 0.5f * std::sin(2.0f * PI * 28.0f * t));
            data[i] = (pulse * 0.75f + rattle) * env * 0.50f;
        }
        m_samples[static_cast<size_t>(SoundId::PlayerBurp)].data = std::move(data);
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

void AudioEngine::synthesizeMenuMusic() {
    // 16-second uplifting, cheerful title melody (Cmaj7 -> Gadd9 -> Am9 -> Fmaj7)
    constexpr float totalSecs = 16.0f;
    constexpr size_t totalFrames = static_cast<size_t>(totalSecs * SAMPLE_RATE);
    std::vector<float> pcm(totalFrames * 2, 0.0f);

    struct Note {
        float freq;
        float startTime;
        float duration;
        float pan; // -1.0 (L) to +1.0 (R)
        float amp;
    };

    const std::vector<Note> notes = {
        // Section 1: Cmaj7 (0.0s - 4.0s)
        { 130.81f, 0.0f, 3.8f,  0.0f, 0.38f }, // C3 root
        { 196.00f, 0.2f, 3.6f, -0.3f, 0.30f }, // G3
        { 246.94f, 0.5f, 3.2f,  0.3f, 0.28f }, // B3
        { 329.63f, 0.9f, 2.8f,  0.1f, 0.24f }, // E4
        { 392.00f, 1.4f, 2.3f, -0.2f, 0.22f }, // G4
        { 493.88f, 2.0f, 1.8f,  0.2f, 0.20f }, // B4
        { 523.25f, 2.6f, 1.3f, -0.1f, 0.18f }, // C5

        // Section 2: Gadd9 (4.0s - 8.0s)
        {  98.00f, 4.0f, 3.8f,  0.0f, 0.40f }, // G2 root
        { 146.83f, 4.2f, 3.6f,  0.3f, 0.30f }, // D3
        { 220.00f, 4.5f, 3.2f, -0.3f, 0.28f }, // A3
        { 293.66f, 4.9f, 2.8f,  0.2f, 0.25f }, // D4
        { 392.00f, 5.4f, 2.3f, -0.1f, 0.22f }, // G4
        { 440.00f, 6.0f, 1.8f,  0.1f, 0.20f }, // A4
        { 587.33f, 6.6f, 1.3f, -0.2f, 0.18f }, // D5

        // Section 3: Am9 (8.0s - 12.0s)
        { 110.00f, 8.0f, 3.8f,  0.0f, 0.38f }, // A2 root
        { 164.81f, 8.2f, 3.6f, -0.3f, 0.30f }, // E3
        { 220.00f, 8.5f, 3.2f,  0.3f, 0.28f }, // A3
        { 261.63f, 8.9f, 2.8f, -0.2f, 0.25f }, // C4
        { 329.63f, 9.4f, 2.3f,  0.2f, 0.22f }, // E4
        { 493.88f, 10.0f, 1.8f, -0.1f, 0.20f }, // B4
        { 659.25f, 10.6f, 1.3f,  0.1f, 0.18f }, // E5

        // Section 4: Fmaj7 (12.0s - 16.0s)
        {  87.31f, 12.0f, 3.8f,  0.0f, 0.40f }, // F2 root
        { 130.81f, 12.2f, 3.6f,  0.3f, 0.30f }, // C3
        { 174.61f, 12.5f, 3.2f, -0.3f, 0.28f }, // F3
        { 220.00f, 12.9f, 2.8f,  0.1f, 0.25f }, // A3
        { 329.63f, 13.4f, 2.3f, -0.2f, 0.22f }, // E4
        { 349.23f, 14.0f, 1.8f,  0.2f, 0.20f }, // F4
        { 523.25f, 14.6f, 1.3f, -0.1f, 0.18f }  // C5
    };

    for (const auto& note : notes) {
        const size_t startFrame = static_cast<size_t>(note.startTime * SAMPLE_RATE);
        const size_t noteFrames = static_cast<size_t>(note.duration * SAMPLE_RATE);
        const float volL = note.amp * std::clamp(1.0f - note.pan * 0.6f, 0.0f, 1.0f);
        const float volR = note.amp * std::clamp(1.0f + note.pan * 0.6f, 0.0f, 1.0f);

        for (size_t f = 0; f < noteFrames; ++f) {
            const size_t targetFrame = (startFrame + f) % totalFrames;
            const float t = static_cast<float>(f) / SAMPLE_RATE;
            const float env = std::sin(std::min(1.0f, t * 16.0f) * (PI * 0.5f)) * std::exp(-t * 1.15f);
            const float harmonic1 = std::sin(2.0f * PI * note.freq * t);
            const float harmonic2 = 0.42f * std::sin(2.0f * PI * note.freq * 2.0f * t);
            const float harmonic3 = 0.18f * std::sin(2.0f * PI * note.freq * 3.0f * t);
            const float wave = (harmonic1 + harmonic2 + harmonic3) * env;

            pcm[targetFrame * 2 + 0] += wave * volL * 0.45f;
            pcm[targetFrame * 2 + 1] += wave * volR * 0.45f;
        }
    }

    // Smooth seamless loop crossfade
    const size_t crossfadeFrames = static_cast<size_t>(1.5f * SAMPLE_RATE);
    for (size_t f = 0; f < crossfadeFrames; ++f) {
        const float alpha = static_cast<float>(f) / static_cast<float>(crossfadeFrames);
        const size_t tailIdx = (totalFrames - crossfadeFrames + f) * 2;
        const size_t headIdx = f * 2;
        pcm[headIdx + 0] = glm::mix(pcm[headIdx + 0], pcm[tailIdx + 0], 1.0f - alpha);
        pcm[headIdx + 1] = glm::mix(pcm[headIdx + 1], pcm[tailIdx + 1], 1.0f - alpha);
    }

    {
        std::lock_guard<std::mutex> lock(m_musicMutex);
        m_menuMusicPcm = std::move(pcm);
        m_menuMusicPos = 0.0f;
        m_menuMusicLoaded = true;
    }
    log::info("Procedural Main Menu background music synthesized (16.0s melodic arpeggio loop)");
}

void AudioEngine::synthesizeGameMusic() {
    // 20-second calming, mystical ambient in-game exploration pads (D Dorian / Dm9 / G / Am)
    constexpr float totalSecs = 20.0f;
    constexpr size_t totalFrames = static_cast<size_t>(totalSecs * SAMPLE_RATE);
    std::vector<float> pcm(totalFrames * 2, 0.0f);

    struct Note {
        float freq;
        float startTime;
        float duration;
        float pan;
        float amp;
    };

    const std::vector<Note> notes = {
        // Pad 1: Dm9 (0.0s - 6.5s)
        {  73.42f, 0.0f, 6.2f,  0.0f, 0.42f }, // D2 sub
        { 110.00f, 0.3f, 5.9f, -0.3f, 0.32f }, // A2
        { 146.83f, 0.6f, 5.6f,  0.3f, 0.28f }, // D3
        { 174.61f, 1.0f, 5.2f, -0.2f, 0.25f }, // F3
        { 261.63f, 1.5f, 4.7f,  0.2f, 0.22f }, // C4
        { 329.63f, 2.2f, 4.0f, -0.1f, 0.20f }, // E4
        { 587.33f, 3.2f, 3.0f,  0.1f, 0.16f }, // D5 (high flute chime)

        // Pad 2: Gsus2 / B (6.5s - 13.0s)
        {  61.74f, 6.5f, 6.2f,  0.0f, 0.40f }, // B1
        {  98.00f, 6.8f, 5.9f,  0.3f, 0.32f }, // G2
        { 146.83f, 7.2f, 5.5f, -0.3f, 0.28f }, // D3
        { 220.00f, 7.8f, 4.9f,  0.2f, 0.24f }, // A3
        { 293.66f, 8.5f, 4.2f, -0.2f, 0.22f }, // D4
        { 440.00f, 9.5f, 3.2f,  0.2f, 0.18f }, // A4

        // Pad 3: Am11 (13.0s - 20.0s)
        {  55.00f, 13.0f, 6.7f,  0.0f, 0.42f }, // A1 deep bass
        {  82.41f, 13.3f, 6.4f, -0.3f, 0.32f }, // E2
        { 130.81f, 13.7f, 6.0f,  0.3f, 0.28f }, // C3
        { 196.00f, 14.3f, 5.4f, -0.2f, 0.25f }, // G3
        { 293.66f, 15.0f, 4.7f,  0.2f, 0.22f }, // D4
        { 329.63f, 16.0f, 3.7f, -0.1f, 0.20f }, // E4
        { 523.25f, 17.0f, 2.7f,  0.1f, 0.16f }  // C5
    };

    for (const auto& note : notes) {
        const size_t startFrame = static_cast<size_t>(note.startTime * SAMPLE_RATE);
        const size_t noteFrames = static_cast<size_t>(note.duration * SAMPLE_RATE);
        const float volL = note.amp * std::clamp(1.0f - note.pan * 0.6f, 0.0f, 1.0f);
        const float volR = note.amp * std::clamp(1.0f + note.pan * 0.6f, 0.0f, 1.0f);

        for (size_t f = 0; f < noteFrames; ++f) {
            const size_t targetFrame = (startFrame + f) % totalFrames;
            const float t = static_cast<float>(f) / SAMPLE_RATE;
            // Soft slow attack and lingering release for ethereal ambient pads
            const float env = std::sin(std::min(1.0f, t * 1.5f) * (PI * 0.5f)) * std::exp(-t * 0.45f);
            const float lfo = 1.0f + 0.004f * std::sin(2.0f * PI * 0.4f * t); // subtle warm chorus pitch drift
            const float harmonic1 = std::sin(2.0f * PI * (note.freq * lfo) * t);
            const float harmonic2 = 0.25f * std::sin(2.0f * PI * (note.freq * 2.0f) * t);
            const float harmonic3 = 0.08f * std::sin(2.0f * PI * (note.freq * 3.0f) * t);
            const float wave = (harmonic1 + harmonic2 + harmonic3) * env;

            pcm[targetFrame * 2 + 0] += wave * volL * 0.45f;
            pcm[targetFrame * 2 + 1] += wave * volR * 0.45f;
        }
    }

    // Smooth seamless loop crossfade (last 2.0 seconds)
    const size_t crossfadeFrames = static_cast<size_t>(2.0f * SAMPLE_RATE);
    for (size_t f = 0; f < crossfadeFrames; ++f) {
        const float alpha = static_cast<float>(f) / static_cast<float>(crossfadeFrames);
        const size_t tailIdx = (totalFrames - crossfadeFrames + f) * 2;
        const size_t headIdx = f * 2;
        pcm[headIdx + 0] = glm::mix(pcm[headIdx + 0], pcm[tailIdx + 0], 1.0f - alpha);
        pcm[headIdx + 1] = glm::mix(pcm[headIdx + 1], pcm[tailIdx + 1], 1.0f - alpha);
    }

    {
        std::lock_guard<std::mutex> lock(m_musicMutex);
        m_gameMusicPcm = std::move(pcm);
        m_gameMusicPos = 0.0f;
        m_gameMusicLoaded = true;
    }
    log::info("Procedural In-Game ambient exploration music synthesized (20.0s atmospheric pad loop)");
}

} // namespace vox

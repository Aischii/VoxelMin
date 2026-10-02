#include "audio/AudioEngine.hpp"
#include "core/Log.hpp"
#include "world/World.hpp"
#include "world/Block.hpp"

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

void AudioEngine::initReverb() {
    // Schroeder / Freeverb tuned delay line sample buffer lengths
    const size_t combLengthsL[NUM_COMBS] = { 1116, 1188, 1277, 1356 };
    const size_t combLengthsR[NUM_COMBS] = { 1139, 1211, 1300, 1379 };
    const size_t allPassLengthsL[NUM_ALLPASS] = { 225, 556 };
    const size_t allPassLengthsR[NUM_ALLPASS] = { 248, 579 };

    for (size_t i = 0; i < NUM_COMBS; ++i) {
        m_combsL[i].init(combLengthsL[i]);
        m_combsR[i].init(combLengthsR[i]);
    }
    for (size_t i = 0; i < NUM_ALLPASS; ++i) {
        m_allPassL[i].init(allPassLengthsL[i]);
        m_allPassR[i].init(allPassLengthsR[i]);
    }

    m_underwaterFilterL.setCutoff(420.0f);
    m_underwaterFilterR.setCutoff(420.0f);
}

bool AudioEngine::init() {
    if (m_initialized) return true;

    initReverb();
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
    log::info("Audio engine initialised (Sound Physics DSP, 44.1kHz stereo, %d sound banks, BGM active)",
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
    synthesizeBackroomsMusic();
}

void AudioEngine::updateEnvironment(const World& world, const glm::vec3& listenerPos, bool isUnderwater) {
    std::lock_guard<std::mutex> lock(m_envMutex);
    m_isUnderwater = isUnderwater;

    // 14 acoustic probe rays
    const glm::vec3 PROBE_DIRS[14] = {
        { 1.0f,  0.0f,  0.0f}, {-1.0f,  0.0f,  0.0f},
        { 0.0f,  1.0f,  0.0f}, { 0.0f, -1.0f,  0.0f},
        { 0.0f,  0.0f,  1.0f}, { 0.0f,  0.0f, -1.0f},
        { 0.707f, 0.707f, 0.0f}, {-0.707f, 0.707f, 0.0f},
        { 0.707f,-0.707f, 0.0f}, {-0.707f,-0.707f, 0.0f},
        { 0.0f, 0.707f, 0.707f}, { 0.0f, -0.707f, 0.707f},
        { 0.0f, 0.707f,-0.707f}, { 0.0f, -0.707f,-0.707f},
    };

    float totalDist = 0.0f;
    int solidHits = 0;
    float dampSum = 0.0f;

    for (int i = 0; i < 14; ++i) {
        const glm::vec3 dir = PROBE_DIRS[i];
        float dist = 24.0f;
        for (float step = 0.5f; step <= 24.0f; step += 1.0f) {
            const glm::vec3 p = listenerPos + dir * step;
            const int bx = static_cast<int>(std::floor(p.x));
            const int by = static_cast<int>(std::floor(p.y));
            const int bz = static_cast<int>(std::floor(p.z));
            const BlockId b = world.getBlock(bx, by, bz);
            if (isSolid(b)) {
                dist = step;
                solidHits++;
                if (b == BlockId::Leaves || b == BlockId::TallGrass) {
                    dampSum += 0.8f;
                } else if (b == BlockId::Dirt || b == BlockId::Grass || b == BlockId::Sand) {
                    dampSum += 0.5f;
                } else if (b == BlockId::Wood || b == BlockId::Planks) {
                    dampSum += 0.4f;
                } else {
                    dampSum += 0.15f; // Stone / Cobblestone reflective
                }
                break;
            }
        }
        totalDist += dist;
    }

    const float avgDist = totalDist / 14.0f;

    if (solidHits >= 6) {
        // Enclosed space / Cave echo
        const float roomRatio = std::clamp(avgDist / 20.0f, 0.2f, 1.0f);
        m_targetRoomSize = 0.35f + roomRatio * 0.52f;
        m_targetWetMix = std::clamp(0.12f + (static_cast<float>(solidHits) / 14.0f) * 0.36f, 0.10f, 0.48f);
        m_targetDamp = std::clamp(dampSum / static_cast<float>(solidHits), 0.15f, 0.75f);
    } else {
        // Open outdoor plains
        m_targetRoomSize = 0.15f;
        m_targetWetMix = 0.03f;
        m_targetDamp = 0.4f;
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
            m_voices[v].lpfLeft.reset();
            m_voices[v].lpfRight.reset();
            m_voices[v].lpfLeft.setCutoff(20000.0f);
            m_voices[v].lpfRight.setCutoff(20000.0f);
            m_voices[v].isOccluded = false;
            m_voices[v].active = true;
            return;
        }
    }
}

void AudioEngine::play3D(SoundId id, const glm::vec3& worldPos, const glm::vec3& listenerPos,
                        const glm::vec3& listenerFront, float volume, const World* world) {
    if (!m_initialized || id == SoundId::None || m_masterVolume <= 0.001f || m_sfxVolume <= 0.001f) return;

    const glm::vec3 diff = worldPos - listenerPos;
    const float dist = glm::length(diff);
    if (dist > 36.0f) return;

    // Distance attenuation
    const float atten = 1.0f / (1.0f + 0.10f * dist * dist);
    float finalVol = volume * atten;
    if (finalVol < 0.004f) return;

    // Ray-traced acoustic occlusion check
    float occlusionWeight = 0.0f;
    if (world && dist > 0.8f) {
        const glm::vec3 rayDir = diff / dist;
        for (float step = 0.6f; step < dist - 0.4f; step += 0.8f) {
            const glm::vec3 sampleP = listenerPos + rayDir * step;
            const int bx = static_cast<int>(std::floor(sampleP.x));
            const int by = static_cast<int>(std::floor(sampleP.y));
            const int bz = static_cast<int>(std::floor(sampleP.z));
            const BlockId b = world->getBlock(bx, by, bz);
            if (isSolid(b)) {
                if (b == BlockId::Leaves || b == BlockId::TallGrass) {
                    occlusionWeight += 0.3f;
                } else if (b == BlockId::Wood || b == BlockId::Planks) {
                    occlusionWeight += 0.6f;
                } else {
                    occlusionWeight += 1.0f;
                }
            }
        }
    }

    const float cutoffHz = (occlusionWeight > 0.0f) ? std::max(400.0f, 20000.0f / (1.0f + occlusionWeight * 3.2f)) : 20000.0f;
    finalVol *= 1.0f / (1.0f + occlusionWeight * 0.45f);

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
            m_voices[v].lpfLeft.reset();
            m_voices[v].lpfRight.reset();
            m_voices[v].lpfLeft.setCutoff(cutoffHz);
            m_voices[v].lpfRight.setCutoff(cutoffHz);
            m_voices[v].isOccluded = (occlusionWeight > 0.0f);
            m_voices[v].active = true;
            return;
        }
    }
}

void AudioEngine::mixAudio(float* output, size_t frameCount, size_t channels) {
    std::fill(output, output + frameCount * channels, 0.0f);
    if (!m_initialized || m_masterVolume <= 0.001f) return;

    // Smoothly slew reverberation parameters
    {
        std::lock_guard<std::mutex> lock(m_envMutex);
        m_roomSize += (m_targetRoomSize - m_roomSize) * 0.005f;
        m_reverbDamp += (m_targetDamp - m_reverbDamp) * 0.005f;
        m_wetMix += (m_targetWetMix - m_wetMix) * 0.005f;

        for (size_t i = 0; i < NUM_COMBS; ++i) {
            m_combsL[i].feedback = m_roomSize;
            m_combsL[i].damp = m_reverbDamp;
            m_combsR[i].feedback = m_roomSize;
            m_combsR[i].damp = m_reverbDamp;
        }
    }

    // 1. Background music (BGM) with smooth crossfade between Menu and In-Game
    if (m_musicVolume > 0.001f) {
        const float musicVol = m_masterVolume * m_musicVolume * 0.45f;
        const float targetMenuGain = m_inGame ? 0.0f : 1.0f;
        const float targetGameGain = (m_inGame && !m_inBackrooms) ? 1.0f : 0.0f;
        const float targetBackroomsGain = (m_inGame && m_inBackrooms) ? 1.0f : 0.0f;
        const float gainSlew = 1.0f / (1.0f * SAMPLE_RATE);

        std::lock_guard<std::mutex> lock(m_musicMutex);

        const size_t menuLen = m_menuMusicPcm.size() / 2;
        const size_t gameLen = m_gameMusicPcm.size() / 2;
        const size_t backroomsLen = m_backroomsMusicPcm.size() / 2;

        for (size_t f = 0; f < frameCount; ++f) {
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

            if (m_backroomsGain < targetBackroomsGain) {
                m_backroomsGain = std::min(targetBackroomsGain, m_backroomsGain + gainSlew);
            } else if (m_backroomsGain > targetBackroomsGain) {
                m_backroomsGain = std::max(targetBackroomsGain, m_backroomsGain - gainSlew);
            }

            float left = 0.0f;
            float right = 0.0f;

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

            if (m_backroomsGain > 0.001f && backroomsLen > 0) {
                const size_t frameIdx = static_cast<size_t>(m_backroomsMusicPos) % backroomsLen;
                const float bVol = musicVol * m_backroomsGain * 1.25f;
                left += m_backroomsMusicPcm[frameIdx * 2 + 0] * bVol;
                right += m_backroomsMusicPcm[frameIdx * 2 + 1] * bVol;

                m_backroomsMusicPos += 1.0f;
                if (m_backroomsMusicPos >= static_cast<float>(backroomsLen)) {
                    m_backroomsMusicPos -= static_cast<float>(backroomsLen);
                }
            }

            output[f * channels + 0] += left;
            if (channels > 1) {
                output[f * channels + 1] += right;
            }
        }
    }

    // 2. Seamless ambient wind / underwater ambience
    if (m_inGame && m_ambientVolume > 0.001f) {
        const float ambVol = m_masterVolume * m_ambientVolume;

        if (m_isUnderwater && !m_underwaterLoop.empty()) {
            const size_t underLen = m_underwaterLoop.size();
            for (size_t f = 0; f < frameCount; ++f) {
                size_t idx = static_cast<size_t>(m_underwaterPos);
                float sample = m_underwaterLoop[idx % underLen] * ambVol * 0.9f;
                m_underwaterPos += 1.0f;
                if (m_underwaterPos >= static_cast<float>(underLen)) {
                    m_underwaterPos -= static_cast<float>(underLen);
                }
                output[f * channels + 0] += sample;
                if (channels > 1) output[f * channels + 1] += sample;
            }
        } else if (!m_windLoop.empty()) {
            const size_t windLen = m_windLoop.size();
            for (size_t f = 0; f < frameCount; ++f) {
                size_t idx = static_cast<size_t>(m_windPos);
                float sample = m_windLoop[idx % windLen] * ambVol;
                m_windPos += 1.0f;
                if (m_windPos >= static_cast<float>(windLen)) {
                    m_windPos -= static_cast<float>(windLen);
                }
                output[f * channels + 0] += sample;
                if (channels > 1) output[f * channels + 1] += sample;
            }
        }
    }

    // 3. Active polyphonic SFX voices + Reverb Delay Network
    if (m_sfxVolume > 0.001f) {
        const float sfxMaster = m_masterVolume * m_sfxVolume;
        std::vector<float> reverbInL(frameCount, 0.0f);
        std::vector<float> reverbInR(frameCount, 0.0f);

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

                const float rawSample = sampleData[idx];
                const float filteredL = voice.lpfLeft.process(rawSample);
                const float filteredR = voice.lpfRight.process(rawSample);

                const float dryL = filteredL * voice.volumeLeft * sfxMaster;
                const float dryR = filteredR * voice.volumeRight * sfxMaster;

                output[f * channels + 0] += dryL;
                if (channels > 1) output[f * channels + 1] += dryR;

                // Send to environmental reverb bus (scaled by direct gain)
                reverbInL[f] += dryL * 0.6f;
                reverbInR[f] += dryR * 0.6f;

                voice.samplePos += voice.pitch;
            }
        }

        // Process Reverb Delay Network (4 Parallel Combs + 2 Series All-Pass per channel)
        if (m_wetMix > 0.001f) {
            for (size_t f = 0; f < frameCount; ++f) {
                // Comb bank L & R
                float combOutL = 0.0f;
                float combOutR = 0.0f;
                for (size_t c = 0; c < NUM_COMBS; ++c) {
                    combOutL += m_combsL[c].process(reverbInL[f]);
                    combOutR += m_combsR[c].process(reverbInR[f]);
                }

                // Allpass cascade L & R
                float allPassOutL = combOutL * 0.25f;
                float allPassOutR = combOutR * 0.25f;
                for (size_t a = 0; a < NUM_ALLPASS; ++a) {
                    allPassOutL = m_allPassL[a].process(allPassOutL);
                    allPassOutR = m_allPassR[a].process(allPassOutR);
                }

                output[f * channels + 0] += allPassOutL * m_wetMix;
                if (channels > 1) output[f * channels + 1] += allPassOutR * m_wetMix;
            }
        }
    }

    // 4. Underwater global low-pass filter
    if (m_isUnderwater) {
        for (size_t f = 0; f < frameCount; ++f) {
            output[f * channels + 0] = m_underwaterFilterL.process(output[f * channels + 0]);
            if (channels > 1) {
                output[f * channels + 1] = m_underwaterFilterR.process(output[f * channels + 1]);
            }
        }
    }

    // 5. Soft limiter to prevent clipping
    for (size_t i = 0; i < frameCount * channels; ++i) {
        output[i] = std::clamp(output[i], -1.0f, 1.0f);
    }
}

void AudioEngine::precomputeSounds() {
    m_samples.resize(static_cast<size_t>(SoundId::Count));

    // SoundId::Click
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

    // SoundId::ItemPickup
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

    // SoundId::StepGrass
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

    // SoundId::StepStone
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

    // SoundId::StepWood
    {
        const int n = static_cast<int>(0.085f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(555);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float freq = 290.0f - 110.0f * (t / 0.085f);
            const float env = std::exp(-t * 45.0f);
            const float tone = std::sin(2.0f * PI * freq * t);
            const float noise = dist(rng) * std::exp(-t * 80.0f);
            data[i] = (tone * 0.72f + noise * 0.28f) * env * 0.40f;
        }
        m_samples[static_cast<size_t>(SoundId::StepWood)].data = std::move(data);
    }

    // SoundId::DigGrass
    {
        const int n = static_cast<int>(0.07f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(777);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        float lp = 0.0f;
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 70.0f);
            const float noise = dist(rng);
            lp += (noise - lp) * 0.35f;
            data[i] = lp * env * 0.35f;
        }
        m_samples[static_cast<size_t>(SoundId::DigGrass)].data = std::move(data);
    }

    // SoundId::DigStone
    {
        const int n = static_cast<int>(0.06f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(888);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 110.0f);
            const float tone = 0.6f * std::sin(2.0f * PI * 2100.0f * t) + 0.4f * std::sin(2.0f * PI * 850.0f * t);
            const float noise = dist(rng);
            data[i] = (tone * 0.65f + noise * 0.35f) * env * 0.42f;
        }
        m_samples[static_cast<size_t>(SoundId::DigStone)].data = std::move(data);
    }

    // SoundId::DigWood
    {
        const int n = static_cast<int>(0.075f * SAMPLE_RATE);
        std::vector<float> data(n);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 55.0f);
            const float wave = std::sin(2.0f * PI * (380.0f - 140.0f * t) * t);
            data[i] = wave * env * 0.40f;
        }
        m_samples[static_cast<size_t>(SoundId::DigWood)].data = std::move(data);
    }

    // SoundId::PlaceBlock
    {
        const int n = static_cast<int>(0.11f * SAMPLE_RATE);
        std::vector<float> data(n);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 40.0f);
            const float thud = std::sin(2.0f * PI * (190.0f - 90.0f * t) * t);
            data[i] = thud * env * 0.48f;
        }
        m_samples[static_cast<size_t>(SoundId::PlaceBlock)].data = std::move(data);
    }

    // SoundId::MobHurt
    {
        const int n = static_cast<int>(0.22f * SAMPLE_RATE);
        std::vector<float> data(n);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float norm = t / 0.22f;
            const float freq = 160.0f + 120.0f * std::sin(norm * PI * 1.5f);
            const float env = std::sin(norm * PI) * std::exp(-norm * 2.5f);
            const float wave = 0.5f * std::sin(2.0f * PI * freq * t) + 0.3f * std::sin(2.0f * PI * freq * 2.0f * t);
            data[i] = wave * env * 0.55f;
        }
        m_samples[static_cast<size_t>(SoundId::MobHurt)].data = std::move(data);
    }

    // SoundId::ToolBreak
    {
        const int n = static_cast<int>(0.35f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(999);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 12.0f);
            const float snap = std::sin(2.0f * PI * 1200.0f * t) * std::exp(-t * 90.0f);
            const float shatter = dist(rng) * std::exp(-t * 22.0f);
            data[i] = (snap * 0.6f + shatter * 0.4f) * env * 0.50f;
        }
        m_samples[static_cast<size_t>(SoundId::ToolBreak)].data = std::move(data);
    }

    // SoundId::PlayerHurt
    {
        const int n = static_cast<int>(0.20f * SAMPLE_RATE);
        std::vector<float> data(n);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 18.0f);
            const float oof = std::sin(2.0f * PI * (180.0f - 80.0f * t) * t);
            data[i] = oof * env * 0.60f;
        }
        m_samples[static_cast<size_t>(SoundId::PlayerHurt)].data = std::move(data);
    }

    // SoundId::PlayerEat
    {
        const int n = static_cast<int>(0.16f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(333);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 35.0f);
            const float crunch = dist(rng) * std::exp(-t * 50.0f);
            const float chew = std::sin(2.0f * PI * 220.0f * t);
            data[i] = (crunch * 0.7f + chew * 0.3f) * env * 0.45f;
        }
        m_samples[static_cast<size_t>(SoundId::PlayerEat)].data = std::move(data);
    }

    // SoundId::PlayerBurp
    {
        const int n = static_cast<int>(0.38f * SAMPLE_RATE);
        std::vector<float> data(n);
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::exp(-t * 8.0f);
            const float rumble = std::sin(2.0f * PI * (95.0f + 25.0f * std::sin(2.0f * PI * 18.0f * t)) * t);
            data[i] = rumble * env * 0.50f;
        }
        m_samples[static_cast<size_t>(SoundId::PlayerBurp)].data = std::move(data);
    }

    // SoundId::WaterSplash (Dynamic fluid impact splash)
    {
        const int n = static_cast<int>(0.32f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(404);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        float lp = 0.0f;
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::pow(t / 0.32f, 0.2f) * std::exp(-t * 14.0f);
            const float noise = dist(rng);
            lp += (noise - lp) * (0.6f - 0.4f * (t / 0.32f));
            const float slap = 0.45f * std::sin(2.0f * PI * 140.0f * t) * std::exp(-t * 35.0f);
            data[i] = (lp * 0.7f + slap) * env * 0.52f;
        }
        m_samples[static_cast<size_t>(SoundId::WaterSplash)].data = std::move(data);
    }

    // SoundId::WaterFlow (Soft rushing fluid stream)
    {
        const int n = static_cast<int>(0.45f * SAMPLE_RATE);
        std::vector<float> data(n);
        std::mt19937 rng(505);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        float lp = 0.0f;
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float env = std::sin((t / 0.45f) * PI);
            const float noise = dist(rng);
            lp += (noise - lp) * 0.18f;
            data[i] = lp * env * 0.35f;
        }
        m_samples[static_cast<size_t>(SoundId::WaterFlow)].data = std::move(data);
    }

    // Ambient Wind Loop (10 seconds)
    {
        const int n = 10 * SAMPLE_RATE;
        m_windLoop.resize(n);
        std::mt19937 rng(101);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        float lp1 = 0.0f;
        float lp2 = 0.0f;
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float swell = 0.6f + 0.4f * std::sin(2.0f * PI * 0.1f * t) * std::cos(2.0f * PI * 0.035f * t);
            const float noise = dist(rng);
            lp1 += (noise - lp1) * 0.025f;
            lp2 += (lp1 - lp2) * 0.015f;
            m_windLoop[i] = lp2 * swell * 0.22f;
        }
    }

    // Ambient Underwater Bubbling Loop (8 seconds)
    {
        const int n = 8 * SAMPLE_RATE;
        m_underwaterLoop.resize(n);
        std::mt19937 rng(202);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        float lp = 0.0f;
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / SAMPLE_RATE;
            const float noise = dist(rng);
            lp += (noise - lp) * 0.008f;
            const float bubble1 = 0.08f * std::sin(2.0f * PI * (240.0f + 30.0f * std::sin(2.0f * PI * 1.5f * t)) * t);
            const float bubble2 = 0.06f * std::sin(2.0f * PI * (360.0f + 45.0f * std::cos(2.0f * PI * 2.2f * t)) * t);
            m_underwaterLoop[i] = (lp * 0.6f + bubble1 + bubble2) * 0.32f;
        }
    }
}

void AudioEngine::synthesizeMenuMusic() {
    // 16-second nostalgic procedural title arpeggio
    const int n = 16 * SAMPLE_RATE;
    m_menuMusicPcm.assign(n * 2, 0.0f);

    const float notes[16] = {
        261.63f, 329.63f, 392.00f, 523.25f, // C4 - E4 - G4 - C5
        293.66f, 349.23f, 440.00f, 587.33f, // D4 - F4 - A4 - D5
        329.63f, 392.00f, 493.88f, 659.25f, // E4 - G4 - B4 - E5
        261.63f, 329.63f, 392.00f, 523.25f  // C4 - E4 - G4 - C5
    };

    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / SAMPLE_RATE;
        const int noteIdx = static_cast<int>(t / 1.0f) % 16;
        const float noteT = std::fmod(t, 1.0f);
        const float env = std::exp(-noteT * 3.2f);
        const float freq = notes[noteIdx];

        const float wave = 0.65f * std::sin(2.0f * PI * freq * t)
                         + 0.25f * std::sin(2.0f * PI * freq * 2.0f * t)
                         + 0.10f * std::sin(2.0f * PI * freq * 0.5f * t);

        const float sample = wave * env * 0.32f;
        m_menuMusicPcm[i * 2 + 0] = sample;
        m_menuMusicPcm[i * 2 + 1] = sample;
    }
    m_menuMusicLoaded = true;
}

void AudioEngine::synthesizeGameMusic() {
    // 24-second ambient exploration soundscape
    const int n = 24 * SAMPLE_RATE;
    m_gameMusicPcm.assign(n * 2, 0.0f);

    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / SAMPLE_RATE;
        const float chord1 = std::sin(2.0f * PI * 130.81f * t) + std::sin(2.0f * PI * 164.81f * t) + std::sin(2.0f * PI * 196.00f * t);
        const float chord2 = std::sin(2.0f * PI * 146.83f * t) + std::sin(2.0f * PI * 174.61f * t) + std::sin(2.0f * PI * 220.00f * t);
        const float blend = 0.5f + 0.5f * std::sin(2.0f * PI * (t / 24.0f));
        const float pad = (chord1 * (1.0f - blend) + chord2 * blend) * 0.12f;

        m_gameMusicPcm[i * 2 + 0] = pad;
        m_gameMusicPcm[i * 2 + 1] = pad;
    }
    m_gameMusicLoaded = true;
}

void AudioEngine::synthesizeBackroomsMusic() {
    // 32-second haunting, unsettling liminal horror ambient drone
    const int n = 32 * SAMPLE_RATE;
    m_backroomsMusicPcm.assign(n * 2, 0.0f);

    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / SAMPLE_RATE;

        // 1. 60Hz Fluorescent Ballast Hum with harmonic saturation & electrical micro-jitter
        const float humPhaseMod = 0.08f * std::sin(2.0f * PI * 7.3f * t);
        const float hum60 = std::sin(2.0f * PI * 60.0f * t + humPhaseMod);
        const float hum120 = 0.45f * std::sin(2.0f * PI * 120.0f * t);
        const float hum180 = 0.25f * std::sin(2.0f * PI * 180.0f * t);
        const float hum300 = 0.12f * std::sin(2.0f * PI * 300.0f * t);
        const float ballastBuzz = (hum60 + hum120 + hum180 + hum300) * 0.16f;

        // 2. Unsettling Dissonant Minor 2nd & Tritone Drones (D#2 = 77.78Hz, A2 = 110.0Hz, C3 = 130.81Hz, F#3 = 185.0Hz)
        const float drone1 = std::sin(2.0f * PI * 77.78f * t);
        const float drone2 = std::sin(2.0f * PI * 110.00f * t);
        const float drone3 = std::sin(2.0f * PI * 130.81f * t);
        const float drone4 = 0.6f * std::sin(2.0f * PI * 185.00f * t);
        const float swell = 0.5f + 0.5f * std::sin(2.0f * PI * (t / 16.0f));
        const float eeriePad = (drone1 * 0.4f + drone2 * 0.35f + drone3 * 0.25f + drone4 * swell * 0.3f) * 0.14f;

        // 3. Phasing Metallic Liminal Resonance
        const float phaseFreq = 220.0f + 18.0f * std::sin(2.0f * PI * 0.15f * t);
        const float metallic = 0.08f * std::sin(2.0f * PI * phaseFreq * t) * (0.4f + 0.6f * std::cos(2.0f * PI * 0.08f * t));

        // 4. Subtle flyback resonance / fluorescent whine (12 kHz)
        const float whine = 0.015f * std::sin(2.0f * PI * 12400.0f * t);

        const float monoSignal = ballastBuzz + eeriePad + metallic + whine;

        // Subtle slow stereo panning drift
        const float panL = 0.5f + 0.3f * std::sin(2.0f * PI * (t / 11.0f));
        const float panR = 0.5f - 0.3f * std::sin(2.0f * PI * (t / 11.0f));

        m_backroomsMusicPcm[i * 2 + 0] = monoSignal * panL * 1.35f;
        m_backroomsMusicPcm[i * 2 + 1] = monoSignal * panR * 1.35f;
    }
    m_backroomsMusicLoaded = true;
}

} // namespace vox

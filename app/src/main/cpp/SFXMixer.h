#pragma once

#include <oboe/Oboe.h>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "platform/PlatformServices.h"

class SFXMixer : public oboe::AudioStreamCallback {
public:
    SFXMixer() = default;
    ~SFXMixer() override = default;

    void initialize(IPlatformServices &platformServices, int sampleRate, int channelCount = 1);

    void loadClip(const std::string &clipId, const std::string &assetName);
    void playClip(const std::string &clipId, float volume = 1.0f);

    void playSamples(const float *buffer, size_t length, float volume = 1.0f);

    void stop();
    void resume();
    void shutdown();

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *stream,
                                          void *audioData,
                                          int32_t numFrames) override;

private:
    struct Clip {
        std::vector<float> samples;
    };

    struct SFXInstance {
        const float *buffer = nullptr;
        size_t length = 0;
        size_t cursor = 0;
        float volume = 1.0f;
        bool active = false;
    };

    void ensureStream();
    std::vector<uint8_t> loadAssetToMemory(const std::string &assetName) const;
    std::vector<float> decodeAudio(const std::vector<uint8_t> &bytes,
                                   const std::string &assetName) const;
    std::vector<float> decodeWav(const std::vector<uint8_t> &bytes) const;
    std::vector<float> decodeMp3(const std::vector<uint8_t> &bytes) const;

    IPlatformServices *platformServices_ = nullptr;
    int sampleRate_ = 0;
    int channelCount_ = 0;

    std::shared_ptr<oboe::AudioStream> stream_;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, Clip> clips_;
    std::vector<SFXInstance> activeSFX_;
};

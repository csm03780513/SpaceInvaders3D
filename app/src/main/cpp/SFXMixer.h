#include "oboe/Oboe.h"

struct SFXInstance {
    const float* buffer;
    size_t length;
    size_t cursor;
    float volume;
    bool active;
};

class SFXMixer : public oboe::AudioStreamCallback {
public:
    std::vector<SFXInstance> activeSFX;
    std::shared_ptr<oboe::AudioStream> stream;
    // ... (init and load code)

    // Call this at init (with your desired sample rate and channel count)
    void start(int sampleRate, int channelCount = 1) {
        oboe::AudioStreamBuilder builder;
        builder.setFormat(oboe::AudioFormat::Float)
                ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
                ->setSharingMode(oboe::SharingMode::Shared)
                ->setSampleRate(sampleRate)
                ->setChannelCount(channelCount)
                ->setCallback(this);

        oboe::Result result = builder.openStream(stream);
        if (result != oboe::Result::OK) {
            throw std::runtime_error("Failed to open Oboe SFX stream");
        }
        stream->requestStart();
    }

    void playSFX(const float* buffer, size_t length, float volume=1.0f) {
        // Find a free slot, or add new instance
        for (auto& sfx : activeSFX) {
            if (!sfx.active) {
                sfx = {buffer, length, 0, volume, true};
                return;
            }
        }
        activeSFX.push_back({buffer, length, 0, volume, true});
    }

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream*, void* audioData, int32_t numFrames) override {
        float* out = static_cast<float*>(audioData);
        memset(out, 0, sizeof(float) * numFrames);

        for (auto& sfx : activeSFX) {
            if (!sfx.active) continue;
            size_t remain = sfx.length - sfx.cursor;
            size_t toCopy = std::min<size_t>(numFrames, remain);
            for (size_t i = 0; i < toCopy; ++i) {
                out[i] += sfx.buffer[sfx.cursor + i] * sfx.volume;
            }
            sfx.cursor += toCopy;
            if (sfx.cursor >= sfx.length) sfx.active = false;
        }

        // Optionally clip output if needed
        return oboe::DataCallbackResult::Continue;
    }
};

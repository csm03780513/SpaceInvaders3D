#include <oboe/Oboe.h>

class SimpleSFXPlayer : public oboe::AudioStreamCallback {
public:
    std::vector<float> buffer;
    std::atomic<size_t> playCursor{0};
    std::shared_ptr<oboe::AudioStream> stream;

    void start(int sampleRate) {
        oboe::AudioStreamBuilder builder;
        builder.setFormat(oboe::AudioFormat::Float)
                ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
                ->setSharingMode(oboe::SharingMode::Exclusive)
                ->setChannelCount(1) // Mono for now
                ->setSampleRate(sampleRate)
                ->setCallback(this);

        builder.openStream(stream);
        stream->requestStart();
    }

    void play() { playCursor = 0; }

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream*, void *audioData, int32_t numFrames) override {
        float *out = static_cast<float*>(audioData);
        size_t available = buffer.size() - playCursor;
        size_t toWrite = std::min<size_t>(numFrames, available);

        if (toWrite > 0) {
            memcpy(out, buffer.data() + playCursor, toWrite * sizeof(float));
            playCursor += toWrite;
        }
        if (toWrite < numFrames) {
            memset(out + toWrite, 0, (numFrames - toWrite) * sizeof(float));
        }
        return oboe::DataCallbackResult::Continue;
    }
};

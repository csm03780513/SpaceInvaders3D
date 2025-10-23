#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>

#include "SFXMixer.h"

#include <android/log.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>

namespace {
    constexpr char kAudioAssetPrefix[] = "audio/";

    inline void logError(const char *message) {
        __android_log_print(ANDROID_LOG_ERROR, "SFXMixer", "%s", message);
    }

    inline void logErrorf(const char *fmt, int a, int b) {
        __android_log_print(ANDROID_LOG_ERROR, "SFXMixer", fmt, a, b);
    }
}

void SFXMixer::initialize(AAssetManager *assetManager, int sampleRate, int channelCount) {
    assetManager_ = assetManager;
    sampleRate_ = sampleRate;
    channelCount_ = channelCount;
    ensureStream();
}

void SFXMixer::ensureStream() {
    if (stream_) return;
    if (!assetManager_) {
        throw std::runtime_error("SFXMixer::ensureStream called before initialize");
    }

    oboe::AudioStreamBuilder builder;
    builder.setFormat(oboe::AudioFormat::Float)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Shared)
            ->setSampleRate(sampleRate_)
            ->setChannelCount(channelCount_)
            ->setCallback(this);

    oboe::Result result = builder.openStream(stream_);
    if (result != oboe::Result::OK) {
        throw std::runtime_error("Failed to open Oboe SFX stream");
    }
    stream_->requestStart();
}

void SFXMixer::loadClip(const std::string &clipId, const std::string &assetName) {
    if (!assetManager_) {
        throw std::runtime_error("SFXMixer::loadClip called before initialize");
    }

    auto bytes = loadAssetToMemory(assetName);
    if (bytes.empty()) {
        logError("Failed to load audio asset");
        return;
    }

    auto samples = decodeAudio(bytes, assetName);
    if (samples.empty()) {
        logError("Decoded audio clip is empty");
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    clips_[clipId].samples = std::move(samples);
}

void SFXMixer::playClip(const std::string &clipId, float volume) {
    ensureStream();

    std::lock_guard<std::mutex> lock(mutex_);
    auto clipIt = clips_.find(clipId);
    if (clipIt == clips_.end() || clipIt->second.samples.empty()) {
        return;
    }
    const auto &samples = clipIt->second.samples;
    for (auto &sfx : activeSFX_) {
        if (!sfx.active) {
            sfx = {samples.data(), samples.size(), 0, volume, true};
            return;
        }
    }
    activeSFX_.push_back({samples.data(), samples.size(), 0, volume, true});
}

void SFXMixer::playSamples(const float *buffer, size_t length, float volume) {
    if (!buffer || length == 0) return;
    ensureStream();

    // Guard active list while we modify it.
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &sfx : activeSFX_) {
        if (!sfx.active) {
            sfx = {buffer, length, 0, volume, true};
            return;
        }
    }
    activeSFX_.push_back({buffer, length, 0, volume, true});
}

void SFXMixer::stop() {
    if (stream_) {
        stream_->stop();
    }
}

void SFXMixer::resume() {
    if (stream_) {
        stream_->start();
    }
}

void SFXMixer::shutdown() {
    if (stream_) {
        stream_->close();
        stream_.reset();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    activeSFX_.clear();
    clips_.clear();
}

oboe::DataCallbackResult SFXMixer::onAudioReady(oboe::AudioStream *,
                                                void *audioData,
                                                int32_t numFrames) {
    float *out = static_cast<float *>(audioData);
    std::lock_guard<std::mutex> lock(mutex_);
    std::fill(out, out + numFrames, 0.0f);

    for (auto &sfx : activeSFX_) {
        if (!sfx.active) continue;
        size_t remain = sfx.length - sfx.cursor;
        size_t toCopy = std::min<size_t>(numFrames, remain);
        for (size_t i = 0; i < toCopy; ++i) {
            out[i] += sfx.buffer[sfx.cursor + i] * sfx.volume;
        }
        sfx.cursor += toCopy;
        if (sfx.cursor >= sfx.length) {
            sfx.active = false;
        }
    }

    return oboe::DataCallbackResult::Continue;
}

std::vector<uint8_t> SFXMixer::loadAssetToMemory(const std::string &assetName) const {
    std::string fullPath = assetName;
    if (!assetName.empty() && assetName.find('/') == std::string::npos) {
        fullPath = std::string(kAudioAssetPrefix) + assetName;
    }

    AAsset *asset = AAssetManager_open(assetManager_, fullPath.c_str(), AASSET_MODE_STREAMING);
    if (!asset) {
        logError("Asset not found");
        return {};
    }

    size_t size = AAsset_getLength(asset);
    std::vector<uint8_t> data(size);
    AAsset_read(asset, data.data(), size);
    AAsset_close(asset);
    return data;
}

std::vector<float> SFXMixer::decodeAudio(const std::vector<uint8_t> &bytes,
                                         const std::string &assetName) const {
    if (assetName.size() >= 4) {
        auto ext = assetName.substr(assetName.size() - 4);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".wav") {
            return decodeWav(bytes);
        }
        if (ext == ".mp3") {
            return decodeMp3(bytes);
        }
    }
    logError("Unsupported audio format");
    return {};
}

std::vector<float> SFXMixer::decodeWav(const std::vector<uint8_t> &bytes) const {
    drwav wav;
    if (!drwav_init_memory(&wav, bytes.data(), bytes.size(), nullptr)) {
        logError("Failed to decode WAV");
        return {};
    }

    int channels = wav.channels;
    int sampleRate = wav.sampleRate;
    size_t totalSamples = wav.totalPCMFrameCount * wav.channels;

    std::vector<float> pcm(totalSamples);
    drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, pcm.data());
    drwav_uninit(&wav);

    if (sampleRate != sampleRate_ || channels != channelCount_) {
        logErrorf("WAV clip must match mixer format. channels=%d sampleRate=%d", channels, sampleRate);
    }

    return pcm;
}

std::vector<float> SFXMixer::decodeMp3(const std::vector<uint8_t> &bytes) const {
    drmp3 mp3;
    if (!drmp3_init_memory(&mp3, bytes.data(), bytes.size(), nullptr)) {
        logError("Failed to decode MP3");
        return {};
    }

    drmp3_uint64 frameCount = drmp3_get_pcm_frame_count(&mp3);
    std::vector<float> pcm(frameCount * mp3.channels);
    drmp3_read_pcm_frames_f32(&mp3, frameCount, pcm.data());
    drmp3_uninit(&mp3);

    if (static_cast<int>(mp3.sampleRate) != sampleRate_ || static_cast<int>(mp3.channels) != channelCount_) {
        logErrorf("MP3 clip must match mixer format. channels=%d sampleRate=%d",
                  static_cast<int>(mp3.channels),
                  static_cast<int>(mp3.sampleRate));
    }

    return pcm;
}

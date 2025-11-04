#include "AndroidPlatformServices.h"

#include <android/asset_manager.h>
#include <android/log.h>
#include <android/native_window.h>

#include <algorithm>
#include <chrono>

namespace {
constexpr const char *kLogTag = "PlatformServices";
}

AndroidPlatformServices::AndroidPlatformServices(android_app &app) : app_(app) {}

std::vector<uint8_t> AndroidPlatformServices::loadAsset(const std::string &path) {
    AAssetManager *mgr = app_.activity ? app_.activity->assetManager : nullptr;
    if (!mgr) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "AssetManager unavailable for %s", path.c_str());
        return {};
    }

    std::string fullPath = path;
    if (!path.empty() && path[0] == '/') {
        fullPath = path.substr(1);
    }

    AAsset *asset = AAssetManager_open(mgr, fullPath.c_str(), AASSET_MODE_STREAMING);
    if (!asset) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "Failed to open asset %s", fullPath.c_str());
        return {};
    }

    const off_t length = AAsset_getLength(asset);
    std::vector<uint8_t> data(static_cast<size_t>(length));
    const int64_t read = AAsset_read(asset, data.data(), length);
    AAsset_close(asset);

    if (read < length) {
        data.resize(static_cast<size_t>(std::max<int64_t>(read, 0)));
    }

    return data;
}

WindowInfo AndroidPlatformServices::getWindowInfo() const {
    WindowInfo info{};
    info.nativeWindow = app_.window;
    if (app_.window) {
        info.width = ANativeWindow_getWidth(app_.window);
        info.height = ANativeWindow_getHeight(app_.window);
    }
    return info;
}

void AndroidPlatformServices::setLifecycleCallbacks(const LifecycleCallbacks &callbacks) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    lifecycleCallbacks_ = callbacks;
}

double AndroidPlatformServices::getMonotonicTimeSeconds() const {
    using Clock = std::chrono::steady_clock;
    static const auto start = Clock::now();
    auto now = Clock::now();
    return std::chrono::duration<double>(now - start).count();
}

void AndroidPlatformServices::handleAppCommand(int32_t cmd) {
    LifecycleCallbacks callbacksCopy;
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        callbacksCopy = lifecycleCallbacks_;
    }

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (callbacksCopy.onWindowCreated) {
                callbacksCopy.onWindowCreated();
            }
            break;
        case APP_CMD_TERM_WINDOW:
            if (callbacksCopy.onWindowDestroyed) {
                callbacksCopy.onWindowDestroyed();
            }
            break;
        case APP_CMD_GAINED_FOCUS:
            if (callbacksCopy.onGainedAudioFocus) {
                callbacksCopy.onGainedAudioFocus();
            }
            break;
        case APP_CMD_LOST_FOCUS:
            if (callbacksCopy.onLostAudioFocus) {
                callbacksCopy.onLostAudioFocus();
            }
            break;
        case APP_CMD_CONFIG_CHANGED:
            if (callbacksCopy.onWindowDestroyed) {
                callbacksCopy.onWindowDestroyed();
            }
            break;
        default:
            break;
    }
}

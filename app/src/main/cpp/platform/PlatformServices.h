#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct WindowInfo {
    void *nativeWindow{nullptr};
    int32_t width{0};
    int32_t height{0};
};

struct LifecycleCallbacks {
    std::function<void()> onWindowCreated;
    std::function<void()> onWindowDestroyed;
    std::function<void()> onGainedAudioFocus;
    std::function<void()> onLostAudioFocus;
};

class IPlatformServices {
public:
    virtual ~IPlatformServices() = default;

    virtual std::vector<uint8_t> loadAsset(const std::string &path) = 0;
    virtual WindowInfo getWindowInfo() const = 0;
    virtual void setLifecycleCallbacks(const LifecycleCallbacks &callbacks) = 0;
    virtual double getMonotonicTimeSeconds() const = 0;
};

class DesktopPlatformServices : public IPlatformServices {
public:
    std::vector<uint8_t> loadAsset(const std::string &path) override { (void)path; return {}; }
    WindowInfo getWindowInfo() const override { return {}; }
    void setLifecycleCallbacks(const LifecycleCallbacks &callbacks) override { (void)callbacks; }
    double getMonotonicTimeSeconds() const override { return 0.0; }
};

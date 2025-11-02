#pragma once

#include <android_native_app_glue.h>

#include <functional>
#include <mutex>

#include "PlatformServices.h"

class AndroidPlatformServices : public IPlatformServices {
public:
    explicit AndroidPlatformServices(android_app &app);

    std::vector<uint8_t> loadAsset(const std::string &path) override;
    WindowInfo getWindowInfo() const override;
    void setLifecycleCallbacks(const LifecycleCallbacks &callbacks) override;
    double getMonotonicTimeSeconds() const override;

    void handleAppCommand(int32_t cmd);
    android_app &nativeApp() const { return app_; }

private:
    android_app &app_;
    LifecycleCallbacks lifecycleCallbacks_{};
    mutable std::mutex callbackMutex_;
};

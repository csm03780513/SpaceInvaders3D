//
// Created by carlo on 01/07/2025.
//
#include "GameTime.h"

#include "platform/PlatformServices.h"

#include <algorithm>

float GameTime::deltaTime = 0.0f;

namespace {
bool gInitialized = false;
double gLastTimeSeconds = 0.0;
}

void GameTime::updateTime(IPlatformServices &platformServices) {
    double now = platformServices.getMonotonicTimeSeconds();
    if (!gInitialized) {
        gLastTimeSeconds = now;
        gInitialized = true;
    }

    float actualDeltaTime = static_cast<float>(now - gLastTimeSeconds);
    actualDeltaTime = std::min(actualDeltaTime, 0.0167f);
    gLastTimeSeconds = now;
    deltaTime = actualDeltaTime;
}

GameTime::GameTime() = default;

GameTime::~GameTime() = default;

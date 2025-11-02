//
// Created by carlo on 01/07/2025.
//
#include "GameTime.h"

float GameTime::deltaTime = 0.0f;
using Clock = std::chrono::high_resolution_clock;
static auto lastFrameTime = Clock::now();

void GameTime::updateTime() {
        auto now = Clock::now();
        float actualDeltaTime = std::chrono::duration<float>(now - lastFrameTime).count();
        actualDeltaTime = std::min(actualDeltaTime, 0.0167f);
        lastFrameTime = now;
        deltaTime = actualDeltaTime;
}

GameTime::GameTime() = default;

GameTime::~GameTime() = default;

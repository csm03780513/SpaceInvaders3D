//
// Created by carlo on 01/07/2025.
//
#include "Time.h"

float Time::deltaTime = 0.0f;
using Clock = std::chrono::high_resolution_clock;
static auto lastFrameTime = Clock::now();

void Time::updateTime() {
        auto now = Clock::now();
        float actualDeltaTime = std::chrono::duration<float>(now - lastFrameTime).count();
        actualDeltaTime = std::min(actualDeltaTime, 0.0167f);
        lastFrameTime = now;
        deltaTime = actualDeltaTime;
}

Time::Time() = default;

Time::~Time() = default;

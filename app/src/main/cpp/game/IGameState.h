#pragma once

#include "InputEvent.h"

class RenderContext;

class IGameState {
public:
    virtual ~IGameState() = default;

    virtual void handleInput(const InputEvent &event) = 0;
    virtual void update(float dt) = 0;
    virtual void render(RenderContext &context) = 0;

    virtual void onEnter() {}
    virtual void onExit() {}
};

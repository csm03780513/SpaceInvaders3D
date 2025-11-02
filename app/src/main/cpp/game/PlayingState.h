#pragma once

#include "IGameState.h"

class Game;
class RenderContext;

class PlayingState : public IGameState {
public:
    PlayingState(Game &game, RenderContext &context);

    void handleInput(const InputEvent &event) override;
    void update(float dt) override;
    void render(RenderContext &context) override;
    void onEnter() override;
    void onExit() override;

private:
    Game &game_;
    RenderContext &context_;
    bool touchActive_{false};
    float touchX_{0.0f};
    float touchY_{0.0f};
};

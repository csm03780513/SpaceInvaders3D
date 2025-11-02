#pragma once

#include "IGameState.h"

class Game;
class RenderContext;

class MainMenuState : public IGameState {
public:
    MainMenuState(Game &game, RenderContext &context);

    void handleInput(const InputEvent &event) override;
    void update(float dt) override;
    void render(RenderContext &context) override;
    void onEnter() override;

private:
    Game &game_;
    RenderContext &context_;
};

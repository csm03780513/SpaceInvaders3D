#pragma once

#include "IGameState.h"
#include "../GameObjectData.h"

class Game;
class RenderContext;

class GameOverState : public IGameState {
public:
    GameOverState(Game &game, RenderContext &context);

    void setOutcome(GameState outcome);

    void handleInput(const InputEvent &event) override;
    void update(float dt) override;
    void render(RenderContext &context) override;
    void onEnter() override;

private:
    Game &game_;
    RenderContext &context_;
    GameState outcome_{GameState::Lost};
};

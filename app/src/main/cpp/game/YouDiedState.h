#pragma once

#include "IGameState.h"
#include "../GameObjectData.h"

class Game;
class RenderContext;

class YouDiedState : public IGameState {
public:
    YouDiedState(Game &game, RenderContext &context);

    void setOutcome(GameState outcome);

    void handleInput(const InputEvent &event) override;
    void update(float dt) override;
    void render(RenderContext &context) override;
    void onEnter() override;

private:
    Game &game_;
    RenderContext &context_;
    GameState outcome_{GameState::YouDied};
};

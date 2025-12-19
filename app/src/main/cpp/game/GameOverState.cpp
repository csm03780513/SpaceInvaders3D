#include "GameOverState.h"

#include "Game.h"
#include "RenderContext.h"

GameOverState::GameOverState(Game &game, RenderContext &context)
        : game_(game), context_(context) {}

void GameOverState::setOutcome(GameState outcome) {
    outcome_ = outcome;
}

void GameOverState::handleInput(const InputEvent &event) {
    if (event.type == InputEventType::TouchDown) {
        game_.requestState(GameState::Playing);
    }
}

void GameOverState::update(float dt) {
    (void) dt;
    game_.setRenderState(outcome_);
}

void GameOverState::render(RenderContext &context) {
    context.prepareFrame(false);
    context.drawFrame();
}

void GameOverState::onEnter() {
    game_.setRenderState(outcome_);
}

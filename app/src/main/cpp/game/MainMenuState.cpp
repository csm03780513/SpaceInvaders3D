#include "MainMenuState.h"

#include "Game.h"
#include "RenderContext.h"

MainMenuState::MainMenuState(Game &game, RenderContext &context)
        : game_(game), context_(context) {}

void MainMenuState::handleInput(const InputEvent &event) {
    if (event.type == InputEventType::TouchDown) {
        game_.requestState(GameState::Playing);
    }
}

void MainMenuState::update(float dt) {
    (void) dt;
    context_.setGameState(GameState::MainMenu);
}

void MainMenuState::render(RenderContext &context) {
    context.prepareFrame(false);
    context.drawFrame();
}

void MainMenuState::onEnter() {
    context_.setGameState(GameState::MainMenu);
}

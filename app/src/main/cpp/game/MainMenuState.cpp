#include "MainMenuState.h"

#include "Game.h"
#include "RenderContext.h"

MainMenuState::MainMenuState(Game &game, RenderContext &context)
        : game_(game), context_(context) {}

void MainMenuState::handleInput(const InputEvent &event) {
    if (event.type != InputEventType::TouchDown) return;

    const float halfW = 0.18f * 1.8f; // quad half-width (0.18 from quadVerts * scale.x)
    const float halfH = 0.045f * 0.7f;
    const float cx = 0.0f, cy = 0.15f;
    bool inside = std::abs(event.normalizedX - cx) <= halfW &&
                  std::abs(event.normalizedY - cy) <= halfH;
    if (inside) {
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

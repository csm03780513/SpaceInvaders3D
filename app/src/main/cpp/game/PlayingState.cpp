#include "PlayingState.h"

#include "Game.h"
#include "RenderContext.h"

PlayingState::PlayingState(Game &game, RenderContext &context)
        : game_(game), context_(context) {}

void PlayingState::handleInput(const InputEvent &event) {
    switch (event.type) {
        case InputEventType::TouchDown:
        case InputEventType::TouchMove:
            touchActive_ = true;
            touchX_ = event.normalizedX;
            touchY_ = event.normalizedY;
            context_.setShipPosition(touchX_, touchY_, false);
            break;
        case InputEventType::TouchUp:
        case InputEventType::TouchCancel:
            touchActive_ = false;
            break;
    }
}

void PlayingState::update(float dt) {
    context_.setGameState(GameState::Playing);
    context_.updatePlaying(dt);

    if (touchActive_) {
        context_.setShipPosition(touchX_, touchY_, true);
    }

    if (context_.hasAlienBelow(-0.9f) || context_.hasShipDead()) {
        game_.requestState(GameState::Lost);
    } else if (!context_.hasActiveAliens()) {
        game_.requestState(GameState::Won);
    }
}

void PlayingState::render(RenderContext &context) {
    context.prepareFrame(true);
    context.drawFrame();
}

void PlayingState::onEnter() {
    touchActive_ = false;
    context_.setGameState(GameState::Playing);
}

void PlayingState::onExit() {
    touchActive_ = false;
}

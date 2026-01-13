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
            game_.setShipInput(touchX_, touchY_, false);
            break;
        case InputEventType::TouchUp:
        case InputEventType::TouchCancel:
            touchActive_ = false;
            break;
    }
}

void PlayingState::update(float dt) {
    game_.setRenderState(GameState::Playing);
    game_.updatePlaying(dt);

    if (touchActive_) {
        game_.setShipInput(touchX_, touchY_, true);
    }

    if (game_.hasAlienBelow(-0.9f) || game_.hasShipDead()) {
        game_.requestState(GameState::YouDied);
    } else if (!game_.hasActiveAliens()) {
        game_.requestState(GameState::Won);
    }
}

void PlayingState::render(RenderContext &context) {
    context.prepareFrame(true);
    context.drawFrame();
}

void PlayingState::onEnter() {
    touchActive_ = false;
    game_.setRenderState(GameState::Playing);
}

void PlayingState::onExit() {
    touchActive_ = false;
}

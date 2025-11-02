#include "RenderContext.h"

#include "Renderer.h"

RenderContext::RenderContext(Renderer &renderer) : renderer_(renderer) {}

void RenderContext::updatePlaying(float dt) {
    renderer_.updatePlayingLogic(dt);
}

void RenderContext::prepareFrame(bool isPlaying) {
    renderer_.prepareFrame(isPlaying);
}

void RenderContext::drawFrame() {
    renderer_.drawFrame();
}

void RenderContext::setShipPosition(float x, float y, bool fireBullet) {
    renderer_.setShipPosition(x, y, fireBullet);
}

void RenderContext::restartGame() {
    renderer_.restartGame();
}

void RenderContext::setGameState(GameState state) {
    renderer_.setGameState(state);
}

GameState RenderContext::getGameState() const {
    return renderer_.getGameState();
}

bool RenderContext::hasActiveAliens() const {
    return renderer_.hasActiveAliens();
}

bool RenderContext::hasAlienBelow(float threshold) const {
    return renderer_.hasAlienBelow(threshold);
}

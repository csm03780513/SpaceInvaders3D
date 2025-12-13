#include "RenderContext.h"

#include "Renderer.h"
#include "SimulationScheduler.h"

RenderContext::RenderContext(Renderer &renderer, std::unique_ptr<SimulationScheduler> scheduler)
        : renderer_(renderer), scheduler_(std::move(scheduler)) {}

void RenderContext::tickSimulation(float dt, bool isPlaying) {
    if (scheduler_) {
        scheduler_->tick(dt, isPlaying);
    }
}

void RenderContext::prepareFrame(bool isPlaying) {
    renderer_.prepareFrame(isPlaying);
}

void RenderContext::drawFrame() {
    renderer_.drawFrame();
}

void RenderContext::setShipPosition(float x, float y, bool fireBullet) {
    if (scheduler_) {
        scheduler_->setShipInput(x, y, fireBullet);
    }
}

void RenderContext::restartGame() {
    if (scheduler_) {
        scheduler_->resetWorld();
    }
    renderer_.resetVisuals();
}

void RenderContext::setGameState(GameState state) {
    renderer_.setGameState(state);
}

GameState RenderContext::getGameState() const {
    return renderer_.getGameState();
}

const std::vector<UiEntry> &RenderContext::getUiEntries(TextureSections section) const {
    return renderer_.getUiEntries(section);
}

bool RenderContext::hasActiveAliens() const {
    return renderer_.hasActiveAliens();
}

bool RenderContext::hasAlienBelow(float threshold) const {
    return renderer_.hasAlienBelow(threshold);
}

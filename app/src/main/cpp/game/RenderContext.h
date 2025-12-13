#pragma once

#include <memory>

#include "GameObjectData.h"

class Renderer;
class SimulationScheduler;

class RenderContext {
public:
    RenderContext(Renderer &renderer, std::unique_ptr<SimulationScheduler> scheduler);

    void tickSimulation(float dt, bool isPlaying);
    void prepareFrame(bool isPlaying);
    void drawFrame();

    void setShipPosition(float x, float y, bool fireBullet);
    void restartGame();

    void setGameState(GameState state);
    GameState getGameState() const;
    const std::vector<UiEntry> &getUiEntries(TextureSections section) const;

    bool hasActiveAliens() const;
    bool hasAlienBelow(float threshold) const;

private:
    Renderer &renderer_;
    std::unique_ptr<SimulationScheduler> scheduler_;
};

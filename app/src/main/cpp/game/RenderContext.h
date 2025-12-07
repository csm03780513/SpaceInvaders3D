#pragma once

#include "GameObjectData.h"

class Renderer;

class RenderContext {
public:
    explicit RenderContext(Renderer &renderer);

    void updatePlaying(float dt);
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
};

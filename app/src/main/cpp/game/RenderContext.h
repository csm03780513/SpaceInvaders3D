#pragma once

#include "GameObjectData.h"

class Renderer;

class RenderContext {
public:
    explicit RenderContext(Renderer &renderer);

    void prepareFrame(bool isPlaying);
    void drawFrame();
    const std::vector<UiEntry> &getUiEntries(TextureSections section) const;

private:
    Renderer &renderer_;
};

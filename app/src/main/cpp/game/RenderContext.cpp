#include "RenderContext.h"

#include "Renderer.h"

RenderContext::RenderContext(Renderer &renderer) : renderer_(renderer) {}

void RenderContext::prepareFrame(bool isPlaying) {
    renderer_.prepareFrame(isPlaying);
}

void RenderContext::drawFrame() {
    renderer_.drawFrame();
}

const std::vector<UiEntry> &RenderContext::getUiEntries(TextureSections section) const {
    return renderer_.getUiEntries(section);
}

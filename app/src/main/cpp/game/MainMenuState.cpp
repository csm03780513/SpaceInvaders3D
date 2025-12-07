#include "MainMenuState.h"

#include <cstdlib>

#include "Game.h"
#include "RenderContext.h"
#include "GameObjectData.h"
#include "Util.h"
#include "ButtonBounds.h"

MainMenuState::MainMenuState(Game &game, RenderContext &context)
        : game_(game), context_(context) {}

void MainMenuState::handleInput(const InputEvent &event) {
    if (event.type != InputEventType::TouchDown) return;

    const auto &entries = context_.getUiEntries(TextureSections::MainMenu);
    for (const auto &entry: entries) {
        if (entry.name != "start" && entry.name != "exit") continue;
        const auto widthHeight = Util::getQuadWidthHeight(quadVerts, 6,{entry.scale.x, entry.scale.y});
        const AABB bounds = Collision::getAABB(entry.offset.x, entry.offset.y,widthHeight[0],widthHeight[1]);

        if (!isPointInside(bounds, event.normalizedX, event.normalizedY)) continue;

        if (entry.name == "start") {
            game_.requestState(GameState::Playing);
        } else if (entry.name == "exit") {
            std::exit(EXIT_SUCCESS);
        }
        break;
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

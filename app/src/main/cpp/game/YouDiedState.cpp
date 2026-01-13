#include "YouDiedState.h"

#include "Game.h"
#include "RenderContext.h"
#include "Collision.h"
#include "ButtonBounds.h"
#include "Util.h"

YouDiedState::YouDiedState(Game &game, RenderContext &context)
        : game_(game), context_(context) {}

void YouDiedState::setOutcome(GameState outcome) {
    outcome_ = outcome;
}

void YouDiedState::handleInput(const InputEvent &event) {
    if (event.type == InputEventType::TouchDown) {
        // game_.requestState(GameState::Playing);

        const auto &entries = context_.getUiEntries(TextureSections::YouDied);
        for (const auto &entry: entries) {
            if (entry.name != "restart" && entry.name != "exit") continue;
            const auto widthHeight = Util::getQuadWidthHeight(quadVerts, 6,
                                                              {entry.scale.x, entry.scale.y});
            const AABB bounds = Collision::getAABB(entry.offset.x, entry.offset.y, widthHeight[0],
                                                   widthHeight[1]);
            if (!isPointInside(bounds, event.normalizedX, event.normalizedY)) continue;

            if (entry.name == "restart") {
                game_.requestState(GameState::Playing);
            } else if (entry.name == "exit") {
                std::exit(EXIT_SUCCESS);
            }
            break;
        }
    }
}

void YouDiedState::update(float dt) {
    (void) dt;
    game_.setRenderState(outcome_);
}

void YouDiedState::render(RenderContext &context) {
    context.prepareFrame(false);
    context.drawFrame();
}

void YouDiedState::onEnter() {
    game_.setRenderState(outcome_);
}

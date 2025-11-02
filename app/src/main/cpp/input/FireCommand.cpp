#include "input/FireCommand.h"

#include "game/Game.h"

FireCommand::FireCommand(InputEventType type, float normalizedX, float normalizedY)
        : type_(type), normalizedX_(normalizedX), normalizedY_(normalizedY) {}

void FireCommand::execute(Game &game) {
    InputEvent event{};
    event.type = type_;
    event.normalizedX = normalizedX_;
    event.normalizedY = normalizedY_;
    game.handleInput(event);
}

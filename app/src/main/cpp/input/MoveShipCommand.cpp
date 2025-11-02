#include "input/MoveShipCommand.h"

#include "game/Game.h"

MoveShipCommand::MoveShipCommand(InputEventType type, float normalizedX, float normalizedY)
        : type_(type), normalizedX_(normalizedX), normalizedY_(normalizedY) {}

void MoveShipCommand::execute(Game &game) {
    InputEvent event{};
    event.type = type_;
    event.normalizedX = normalizedX_;
    event.normalizedY = normalizedY_;
    game.handleInput(event);
}

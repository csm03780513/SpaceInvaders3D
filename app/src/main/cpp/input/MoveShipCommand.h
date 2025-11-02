#pragma once

#include "input/InputCommand.h"
#include "game/InputEvent.h"

class MoveShipCommand : public InputCommand {
public:
    MoveShipCommand(InputEventType type, float normalizedX, float normalizedY);
    void execute(Game &game) override;

private:
    InputEventType type_;
    float normalizedX_;
    float normalizedY_;
};

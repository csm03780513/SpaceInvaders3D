#pragma once

#include "input/InputCommand.h"
#include "game/InputEvent.h"

class FireCommand : public InputCommand {
public:
    FireCommand(InputEventType type, float normalizedX, float normalizedY);
    void execute(Game &game) override;

private:
    InputEventType type_;
    float normalizedX_;
    float normalizedY_;
};

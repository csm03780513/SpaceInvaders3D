#pragma once

#include "input/InputCommand.h"

class RestartGameCommand : public InputCommand {
public:
    void execute(Game &game) override;
};

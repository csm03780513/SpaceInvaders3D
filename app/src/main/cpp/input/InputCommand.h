#pragma once

class Game;

class InputCommand {
public:
    virtual ~InputCommand() = default;
    virtual void execute(Game &game) = 0;
};

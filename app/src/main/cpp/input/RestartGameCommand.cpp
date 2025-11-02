#include "input/RestartGameCommand.h"

#include "game/Game.h"
#include "GameObjectData.h"

void RestartGameCommand::execute(Game &game) {
    game.requestState(GameState::Playing);
}

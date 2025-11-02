#pragma once

#include <memory>

#include <android_native_app_glue.h>

#include "GameObjectData.h"
#include "InputEvent.h"

class Renderer;
class RenderContext;
class IGameState;
class MainMenuState;
class PlayingState;
class GameOverState;

class Game {
public:
    explicit Game(android_app *app);
    ~Game();

    void handleCmd(int32_t cmd);
    bool handleInput(const InputEvent &event);

    void update(float dt);
    void render();

    void requestState(GameState state);

private:
    void ensureRenderer();
    void initializeStates();
    void shutdownRenderer();
    void applyPendingStateChange();
    void changeState(GameState state);

    android_app *app_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<RenderContext> renderContext_;

    std::unique_ptr<MainMenuState> mainMenuState_;
    std::unique_ptr<PlayingState> playingState_;
    std::unique_ptr<GameOverState> gameOverState_;

    IGameState *currentState_{nullptr};
    GameState currentStateType_{GameState::MainMenu};

    bool pendingStateChange_{false};
    GameState pendingStateTarget_{GameState::MainMenu};
};

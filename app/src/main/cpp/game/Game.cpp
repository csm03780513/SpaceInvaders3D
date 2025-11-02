#include "Game.h"

#include <android/log.h>
#include <exception>

#include "Renderer.h"
#include "RenderContext.h"
#include "MainMenuState.h"
#include "PlayingState.h"
#include "GameOverState.h"

namespace {
constexpr const char *kLogTag = "SpaceInvaders3D";
}

Game::Game(android_app *app) : app_(app) {}

Game::~Game() {
    shutdownRenderer();
}

void Game::handleCmd(int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            ensureRenderer();
            break;
        case APP_CMD_GAINED_FOCUS:
            if (renderer_) {
                renderer_->resumeAudioPlayer();
            }
            break;
        case APP_CMD_LOST_FOCUS:
            if (renderer_) {
                renderer_->stopAudioPlayer();
            }
            break;
        case APP_CMD_TERM_WINDOW:
            if (renderer_) {
                renderer_->stopAudioPlayer();
            }
            break;
        case APP_CMD_DESTROY:
            shutdownRenderer();
            break;
        default:
            break;
    }
}

bool Game::handleInput(const InputEvent &event) {
    if (!renderContext_ || !currentState_) {
        return false;
    }
    currentState_->handleInput(event);
    return true;
}

void Game::update(float dt) {
    ensureRenderer();
    if (!renderer_ || !currentState_) {
        return;
    }
    currentState_->update(dt);
    applyPendingStateChange();
}

void Game::render() {
    if (!renderer_ || !currentState_) {
        return;
    }
    currentState_->render(*renderContext_);
}

void Game::requestState(GameState state) {
    if (state == currentStateType_) {
        return;
    }
    pendingStateTarget_ = state;
    pendingStateChange_ = true;
}

void Game::ensureRenderer() {
    if (renderer_ || !app_->window) {
        return;
    }
    try {
        renderer_ = std::make_unique<Renderer>(app_);
    } catch (const std::exception &e) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "Renderer init failed: %s", e.what());
        renderer_.reset();
        return;
    }
    renderContext_ = std::make_unique<RenderContext>(*renderer_);
    initializeStates();
    changeState(GameState::MainMenu);
}

void Game::initializeStates() {
    if (!renderContext_) {
        return;
    }
    mainMenuState_ = std::make_unique<MainMenuState>(*this, *renderContext_);
    playingState_ = std::make_unique<PlayingState>(*this, *renderContext_);
    gameOverState_ = std::make_unique<GameOverState>(*this, *renderContext_);
    currentState_ = mainMenuState_.get();
    currentStateType_ = GameState::MainMenu;
    if (currentState_) {
        currentState_->onEnter();
    }
}

void Game::shutdownRenderer() {
    if (currentState_) {
        currentState_->onExit();
    }
    currentState_ = nullptr;
    mainMenuState_.reset();
    playingState_.reset();
    gameOverState_.reset();
    renderContext_.reset();
    renderer_.reset();
    currentStateType_ = GameState::MainMenu;
    pendingStateChange_ = false;
}

void Game::applyPendingStateChange() {
    if (!pendingStateChange_) {
        return;
    }
    changeState(pendingStateTarget_);
    pendingStateChange_ = false;
}

void Game::changeState(GameState state) {
    if (!renderContext_) {
        return;
    }

    if (currentState_) {
        currentState_->onExit();
    }

    IGameState *nextState = nullptr;
    switch (state) {
        case GameState::MainMenu:
            nextState = mainMenuState_.get();
            break;
        case GameState::Playing:
            if (playingState_) {
                renderContext_->restartGame();
            }
            nextState = playingState_.get();
            break;
        case GameState::Won:
        case GameState::Lost:
            if (gameOverState_) {
                gameOverState_->setOutcome(state);
            }
            nextState = gameOverState_.get();
            break;
    }

    currentState_ = nextState;
    currentStateType_ = state;
    if (currentState_) {
        currentState_->onEnter();
    }
}

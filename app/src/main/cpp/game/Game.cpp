#include "Game.h"

#include <android/log.h>
#include <exception>

#include "Renderer.h"
#include "RenderContext.h"
#include "MainMenuState.h"
#include "PlayingState.h"
#include "GameOverState.h"
#include "input/InputCommand.h"
#include "platform/PlatformServices.h"

namespace {
constexpr const char *kLogTag = "SpaceInvaders3D";
}

Game::Game(IPlatformServices &platformServices) : platformServices_(platformServices) {
    LifecycleCallbacks callbacks{};
    callbacks.onWindowCreated = [this]() { onWindowCreated(); };
    callbacks.onWindowDestroyed = [this]() { onWindowDestroyed(); };
    callbacks.onGainedAudioFocus = [this]() { onFocusGained(); };
    callbacks.onLostAudioFocus = [this]() { onFocusLost(); };
    platformServices_.setLifecycleCallbacks(callbacks);
}

Game::~Game() {
    platformServices_.setLifecycleCallbacks({});
    shutdownRenderer();
}

void Game::onWindowCreated() {
    ensureRenderer();
}

void Game::onWindowDestroyed() {
    if (renderer_) {
        renderer_->stopAudioPlayer();
    }
}

void Game::onFocusGained() {
    if (renderer_) {
        renderer_->resumeAudioPlayer();
    }
}

void Game::onFocusLost() {
    if (renderer_) {
        renderer_->stopAudioPlayer();
    }
}

bool Game::handleInput(const InputEvent &event) {
    if (!renderContext_ || !currentState_) {
        return false;
    }
    currentState_->handleInput(event);
    return true;
}

void Game::enqueueCommand(std::unique_ptr<InputCommand> command) {
    if (!command) {
        return;
    }
    std::lock_guard<std::mutex> lock(commandMutex_);
    pendingCommands_.push(std::move(command));
}

GameState Game::getCurrentStateType() const {
    return currentStateType_;
}

void Game::update(float dt) {
    ensureRenderer();
    if (!renderer_ || !currentState_) {
        return;
    }
    processInputCommands();
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
    WindowInfo windowInfo = platformServices_.getWindowInfo();
    if (renderer_ || !windowInfo.nativeWindow) {
        return;
    }
    try {
        renderer_ = std::make_unique<Renderer>(platformServices_);
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

void Game::processInputCommands() {
    std::queue<std::unique_ptr<InputCommand>> commands;
    {
        std::lock_guard<std::mutex> lock(commandMutex_);
        std::swap(commands, pendingCommands_);
    }

    while (!commands.empty()) {
        auto &command = commands.front();
        if (command) {
            command->execute(*this);
        }
        commands.pop();
    }
}

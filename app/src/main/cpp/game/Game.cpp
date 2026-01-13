#include "Game.h"

#include <android/log.h>
#include <exception>

#include "Renderer.h"
#include "RenderContext.h"
#include "MainMenuState.h"
#include "PlayingState.h"
#include "YouDiedState.h"
#include "input/InputCommand.h"
#include "platform/PlatformServices.h"
#include "SFXMixer.h"
#include "Util.h"
#include "PowerUpManager.h"
#include "ParticleSystem.h"
#include "mechanics/AlienManager.h"
#include "mechanics/ProjectileManager.h"
#include "mechanics/CombatEventSubscribers.h"
#include "GameTime.h"

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
    if (renderer_) {
        renderer_->onWindowResumed();
    }
    ensureRenderer();
}

void Game::onWindowDestroyed() {
    if (renderer_) {
        renderer_->onWindowLost();
    }
    if (sfxMixer_) {
        sfxMixer_->stop();
    }
}

void Game::onFocusGained() {
    if (renderer_) {
        renderer_->onWindowResumed();
    }
    if (sfxMixer_) {
        sfxMixer_->resume();
    }
}

void Game::onFocusLost() {
    if (renderer_) {
        renderer_->onWindowLost();
    }
    if (sfxMixer_) {
        sfxMixer_->stop();
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

void Game::setRenderState(GameState state) {
    if (!renderer_) {
        return;
    }
    renderer_->setGameState(state);
}

void Game::setShipInput(float x, float y, bool fireBullet) {
    if (!renderer_ || currentStateType_ != GameState::Playing) {
        return;
    }

    shipManager_.setInput(x, y, fireBullet, projectileManager_.get(), powerUpManager_.get());
}

void Game::updatePlaying(float dt) {
    if (!renderer_) {
        return;
    }

    shipManager_.update(dt);
    MainPushConstants &shipPC = shipManager_.pushConstants();

    if (alienManager_) {
        alienManager_->update(dt);
    }
    if (projectileManager_ && alienManager_) {
        projectileManager_->handleCollisions(alienManager_->aliens(), shipManager_.ship(), shipPC, *alienManager_);
    }
    if (mechanics_) {
        mechanics_->update(dt);
    }
    if (powerUpManager_) {
        powerUpManager_->updatePowerUpData();
        powerUpManager_->checkIfPowerUpCollected(shipManager_.ship());
    }
    if (projectileManager_ && currentStateType_ == GameState::Playing) {
        projectileManager_->update(GameTime::deltaTime);
        if (alienManager_) {
            projectileManager_->tryAlienFire(GameTime::deltaTime, *alienManager_, shipManager_.ship());
        }
    }
}

void Game::restartGame() {
    if (!renderer_) {
        return;
    }

    if (alienManager_) {
        alienManager_->initAliens();
    }

    shipManager_.resetForNewGame(alienManager_ && alienManager_->hasAlienBelow(0.9f));

    if (projectileManager_) {
        projectileManager_->reset();
    }

    renderer_->resetFloatingDamageState();
    setRenderState(GameState::Playing);

}

bool Game::hasActiveAliens() const {
    if (!alienManager_) {
        return false;
    }
    return alienManager_->hasActiveAliens();
}

bool Game::hasAlienBelow(float threshold) const {
    if (!alienManager_) {
        return false;
    }
    return alienManager_->hasAlienBelow(threshold);
}

bool Game::hasShipDead() const {
    return shipManager_.isDead();
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
    initializeSystems();
    initializeStates();
    changeState(GameState::MainMenu);
}

void Game::initializeSystems() {
    if (!renderer_) {
        return;
    }

    if (!sfxMixer_) {
        sfxMixer_ = std::make_shared<SFXMixer>();
        sfxMixer_->initialize(platformServices_, SFX_SAMPLE_RATE, SFX_CHANNELS);
        sfxMixer_->loadClip("shoot", "shoot.wav");
        sfxMixer_->loadClip("explode_1", "explode_1.wav");
        sfxMixer_->loadClip("explode_2", "explode_2.wav");
        sfxMixer_->loadClip("shield", "explode_2.wav");
    }

    if (!util_) {
        VkDevice device = renderer_->device();
        util_ = std::make_shared<Util>(device);
    }

    if (!powerUpManager_) {
        powerUpManager_ = std::make_shared<PowerUpManager>(renderer_->device(), util_, sfxMixer_);
    }
    if (!particleSystem_) {
        particleSystem_ = std::make_unique<ParticleSystem>(renderer_->device(), powerUpManager_);
    }
    if (!alienManager_) {
        alienManager_ = std::make_unique<AlienManager>(powerUpManager_);
    }
    alienManager_->initAliens();
    if (!projectileManager_) {
        projectileManager_ = std::make_unique<ProjectileManager>(eventBus_, *powerUpManager_, *particleSystem_, sfxMixer_);
    }
    if (!mechanics_) {
        mechanics_ = std::make_unique<GameMechanicsCoordinator>(
                eventBus_,
                shipManager_.ship(),
                alienManager_->aliens(),
                *powerUpManager_,
                ailSys_,
                ailRules_,
                shieldRules_,
                actualScore_);
    }

    if (damagePopupSubscriptionId_ == 0) {
        damagePopupSubscriptionId_ = eventBus_.subscribeDamagePopup([this](const DamagePopupSpawned &popup) {
            if (renderer_) {
                renderer_->onDamagePopup(popup);
            }
        });
    }
    if (damageAppliedSubId_ == 0) {
        damageAppliedSubId_ = eventBus_.subscribeDamageApplied([this](const DamageAppliedEvent &event) {
            if (!renderer_) return;
            if (event.killed && event.target != ShipEntityId) {
                renderer_->shakeTimer = 0.12f; // or whatever duration feels right
            }
        });
    }


    renderer_->bindGameplay(&shipManager_.ship(),
                            &shipManager_.pushConstants(),
                            alienManager_.get(),
                            projectileManager_.get(),
                            powerUpManager_.get(),
                            particleSystem_.get(),
                            mechanics_.get(),
                            util_.get());
    renderer_->setScoreSource(&actualScore_);
    renderer_->initializeGameplayResources();

    shipManager_.initialize(util_, sfxMixer_);
}

void Game::initializeStates() {
    if (!renderContext_) {
        return;
    }
    mainMenuState_ = std::make_unique<MainMenuState>(*this, *renderContext_);
    playingState_ = std::make_unique<PlayingState>(*this, *renderContext_);
    youDiedState = std::make_unique<YouDiedState>(*this, *renderContext_);
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
    if (damagePopupSubscriptionId_ != 0) {
        eventBus_.unsubscribeDamagePopup(damagePopupSubscriptionId_);
        damagePopupSubscriptionId_ = 0;
    }

    if(damageAppliedSubId_ != 0) {
        eventBus_.unsubscribeDamageApplied(damageAppliedSubId_);
        damageAppliedSubId_ = 0;
    }
    currentState_ = nullptr;
    mainMenuState_.reset();
    playingState_.reset();
    youDiedState.reset();
    renderContext_.reset();
    renderer_.reset();
    mechanics_.reset();
    projectileManager_.reset();
    alienManager_.reset();
    particleSystem_.reset();
    powerUpManager_.reset();
    util_.reset();
    if (sfxMixer_) {
        sfxMixer_->shutdown();
    }
    sfxMixer_.reset();
    actualScore_ = 0;
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
                restartGame();
            }
            nextState = playingState_.get();
            break;
        case GameState::Won:
        case GameState::YouDied:
            if (youDiedState) {
                youDiedState->setOutcome(state);
            }
            nextState = youDiedState.get();
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

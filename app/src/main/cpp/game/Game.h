#pragma once

#include <memory>
#include <mutex>
#include <queue>

#include "GameObjectData.h"
#include "InputEvent.h"
#include "events/EventBus.h"
#include "ecs/systems/AilmentSystem.h"
#include "ecs/components/CombatComponents.h"
#include "mechanics/ShipManager.h"

class IPlatformServices;

class Renderer;
class RenderContext;
class IGameState;
class MainMenuState;
class PlayingState;
class YouDiedState;
class InputCommand;
class SFXMixer;
class Util;
class PowerUpManager;
class ParticleSystem;
class ProjectileManager;
class AlienManager;
class GameMechanicsCoordinator;

class Game {
public:
    explicit Game(IPlatformServices &platformServices);
    ~Game();

    void onWindowCreated();
    void onWindowDestroyed();
    void onFocusGained();
    void onFocusLost();
    bool handleInput(const InputEvent &event);

    void enqueueCommand(std::unique_ptr<InputCommand> command);
    GameState getCurrentStateType() const;

    void update(float dt);
    void render();

    void requestState(GameState state);
    void setRenderState(GameState state);
    void setShipInput(float x, float y, bool fireBullet);
    void updatePlaying(float dt);
    void restartGame();
    [[nodiscard]] bool hasActiveAliens() const;
    [[nodiscard]] bool hasAlienBelow(float threshold) const;
    [[nodiscard]] bool hasShipDead() const;

private:
    void ensureRenderer();
    void initializeSystems();
    void initializeStates();
    void shutdownRenderer();
    void applyPendingStateChange();
    void changeState(GameState state);
    void processInputCommands();

    IPlatformServices &platformServices_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<RenderContext> renderContext_;

    std::unique_ptr<MainMenuState> mainMenuState_;
    std::unique_ptr<PlayingState> playingState_;
    std::unique_ptr<YouDiedState> youDiedState;

    IGameState *currentState_{nullptr};
    GameState currentStateType_{GameState::MainMenu};

    bool pendingStateChange_{false};
    GameState pendingStateTarget_{GameState::MainMenu};

    std::queue<std::unique_ptr<InputCommand>> pendingCommands_;
    std::mutex commandMutex_;

    std::shared_ptr<SFXMixer> sfxMixer_;
    std::shared_ptr<Util> util_;
    std::shared_ptr<PowerUpManager> powerUpManager_;
    std::unique_ptr<ParticleSystem> particleSystem_;
    std::unique_ptr<AlienManager> alienManager_;
    std::unique_ptr<ProjectileManager> projectileManager_;
    std::unique_ptr<GameMechanicsCoordinator> mechanics_;
    EventBus eventBus_;
    uint32_t damagePopupSubscriptionId_ = 0;
    uint32_t damageAppliedSubId_ = 0;
    uint32_t shipDeadSubId_ = 0;
    AilmentSystem ailSys_;
    AilmentRules ailRules_;
    ShieldRules shieldRules_{ .kineticHalfOnShield = true, .darkMatterIgnoresArmor = true };
    int actualScore_ = 0;
    ShipManager shipManager_{};
};

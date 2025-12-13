#pragma once

#include <vector>

#include "../events/EventBus.h"
#include "../PowerUpManager.h"
#include "../ecs/systems/DamageSystem.h"
#include "../ecs/systems/AilmentSystem.h"
#include "../game/GameWorldManager.h"

class DamageResolver {
public:
    DamageResolver(EventBus &bus,
                   GameWorldManager &world,
                   AilmentRules &ailRules,
                   ShieldRules &shieldRules);
    ~DamageResolver();

    DamageResolver(const DamageResolver &) = delete;
    DamageResolver &operator=(const DamageResolver &) = delete;

private:
    void onHit(const HitEvent &event);

    EventBus &bus_;
    GameWorldManager &world_;
    AilmentRules &ailRules_;
    ShieldRules &shieldRules_;
    DamageSystem damageSystem_;
    uint32_t subscriptionId_ = 0;
};

class AilmentTicker {
public:
    AilmentTicker(EventBus &bus,
                  GameWorldManager &world,
                  AilmentSystem &ailmentSystem);

    void update(float dt);

private:
    EventBus &bus_;
    GameWorldManager &world_;
    AilmentSystem &ailmentSystem_;
    std::vector<DamagePopupSpawned> popupScratch_;
    std::vector<DamageAppliedEvent> appliedScratch_;
};

class PowerUpOnKill {
public:
    PowerUpOnKill(EventBus &bus, GameWorldManager &world, PowerUpManager &manager);
    ~PowerUpOnKill();

    PowerUpOnKill(const PowerUpOnKill &) = delete;
    PowerUpOnKill &operator=(const PowerUpOnKill &) = delete;

private:
    void onDamage(const DamageAppliedEvent &event);

    EventBus &bus_;
    GameWorldManager &world_;
    PowerUpManager &manager_;
    uint32_t subscriptionId_ = 0;
};

class ScoreTracker {
public:
    ScoreTracker(EventBus &bus, GameWorldManager &world, int &actualScore);
    ~ScoreTracker();

    ScoreTracker(const ScoreTracker &) = delete;
    ScoreTracker &operator=(const ScoreTracker &) = delete;

private:
    void onDamage(const DamageAppliedEvent &event);

    EventBus &bus_;
    GameWorldManager &world_;
    int &actualScore_;
    uint32_t subscriptionId_ = 0;
};

class GameMechanicsCoordinator {
public:
    GameMechanicsCoordinator(EventBus &bus,
                             GameWorldManager &world,
                             PowerUpManager &powerUpManager,
                             AilmentSystem &ailmentSystem,
                             AilmentRules &ailRules,
                             ShieldRules &shieldRules,
                             int &actualScore);

    void update(float dt);

private:
    DamageResolver damageResolver_;
    AilmentTicker ailmentTicker_;
    PowerUpOnKill powerUpOnKill_;
    ScoreTracker scoreTracker_;
};

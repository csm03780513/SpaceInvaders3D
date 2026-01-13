#pragma once

#include <span>
#include <vector>

#include "../events/EventBus.h"
#include "../PowerUpManager.h"
#include "../GameObjectData.h"
#include "../ecs/systems/DamageSystem.h"
#include "../ecs/systems/AilmentSystem.h"
#include "AlienManager.h"

class DamageResolver {
public:
    DamageResolver(EventBus &eventBus,
                   Ship &ship,
                   std::span<Alien> aliens,
                   AilmentRules &ailRules,
                   ShieldRules &shieldRules);
    ~DamageResolver();

    DamageResolver(const DamageResolver &) = delete;
    DamageResolver &operator=(const DamageResolver &) = delete;

private:
    void onHit(const HitEvent &event);

    EventBus &eventBus_;
    Ship &ship_;
    std::span<Alien> aliens_;
    AilmentRules &ailRules_;
    ShieldRules &shieldRules_;
    DamageSystem damageSystem_;
    uint32_t subscriptionId_ = 0;
};

class AilmentTicker {
public:
    AilmentTicker(EventBus &bus,
                  Ship &ship,
                  std::span<Alien> aliens,
                  AilmentSystem &ailmentSystem);

    void update(float dt);

private:
    EventBus &bus_;
    Ship &ship_;
    std::span<Alien> aliens_;
    AilmentSystem &ailmentSystem_;
    std::vector<DamagePopupSpawned> popupScratch_;
    std::vector<DamageAppliedEvent> appliedScratch_;
};

class PowerUpOnKill {
public:
    PowerUpOnKill(EventBus &eventBus, std::span<Alien> aliens, PowerUpManager &manager);
    ~PowerUpOnKill();

    PowerUpOnKill(const PowerUpOnKill &) = delete;
    PowerUpOnKill &operator=(const PowerUpOnKill &) = delete;

private:
    void onDamage(const DamageAppliedEvent &event);

    EventBus &bus_;
    std::span<Alien> aliens_;
    PowerUpManager &powerUpManager;
    uint32_t subscriptionId_ = 0;
};

class ScoreTracker {
public:
    ScoreTracker(EventBus &bus, int &actualScore);
    ~ScoreTracker();

    ScoreTracker(const ScoreTracker &) = delete;
    ScoreTracker &operator=(const ScoreTracker &) = delete;

private:
    void onDamage(const DamageAppliedEvent &event);

    EventBus &bus_;
    int &actualScore_;
    uint32_t subscriptionId_ = 0;
};

class GameMechanicsCoordinator {
public:
    GameMechanicsCoordinator(EventBus &bus,
                             Ship &ship,
                             std::span<Alien> aliens,
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

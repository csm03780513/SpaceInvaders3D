#include "CombatEventSubscribers.h"

#include <algorithm>
#include <vector>

DamageResolver::DamageResolver(EventBus &bus,
                               GameWorldManager &world,
                               AilmentRules &ailRules,
                               ShieldRules &shieldRules)
        : bus_(bus), world_(world), ailRules_(ailRules), shieldRules_(shieldRules) {
    damageSystem_.ctx.ailRules = &ailRules_;
    damageSystem_.ctx.shRules = &shieldRules_;
    subscriptionId_ = bus_.subscribeHit([this](const HitEvent &event) { onHit(event); });
}

DamageResolver::~DamageResolver() {
    if (subscriptionId_ != 0) {
        bus_.unsubscribeHit(subscriptionId_);
    }
}

void DamageResolver::onHit(const HitEvent &event) {
    damageSystem_.ctx.ailRules = &ailRules_;
    damageSystem_.ctx.shRules = &shieldRules_;

    std::vector<DamageAppliedEvent> applied;
    std::vector<DamagePopupSpawned> popups;

    auto &w = world_.world();
    auto &ships = w.pool<Ship>();
    auto &aliens = w.pool<Alien>();
    const ecs::EntityId target = static_cast<ecs::EntityId>(event.target);

    if (!w.registry.alive(target)) {
        return;
    }

    if (ships.has(target)) {
        Ship &ship = ships.get(target);
        damageSystem_.apply(event.target,
                            ship.health,
                            ship.resistances,
                            nullptr,
                            ship.ailments,
                            event,
                            applied,
                            popups);
    } else if (aliens.has(target)) {
        Alien &alien = aliens.get(target);
        if (!alien.active) {
            return;
        }
        damageSystem_.apply(event.target,
                            alien.health,
                            alien.resistances,
                            nullptr,
                            alien.ailments,
                            event,
                            applied,
                            popups);
    }

    for (const auto &ev: applied) {
        bus_.publish(ev);
    }
    for (const auto &popup: popups) {
        bus_.publish(popup);
    }
}

AilmentTicker::AilmentTicker(EventBus &bus,
                             GameWorldManager &world,
                             AilmentSystem &ailmentSystem)
        : bus_(bus), world_(world), ailmentSystem_(ailmentSystem) {
    popupScratch_.reserve(32);
    appliedScratch_.reserve(16);
}

void AilmentTicker::update(float dt) {
    popupScratch_.clear();
    appliedScratch_.clear();

    auto &w = world_.world();
    auto &aliens = w.pool<Alien>();
    auto &ships = w.pool<Ship>();
    const auto shipEntity = world_.shipEntity();

    w.registry.forEachAlive([&](ecs::EntityId e) {
        if (aliens.has(e)) {
            Alien &alien = aliens.get(e);
            if (!alien.active) return;
            ailmentSystem_.tick(dt, e, alien.health, alien.ailments, {alien.x, alien.y}, popupScratch_, appliedScratch_);
        }
    });

    if (shipEntity.has_value() && w.registry.alive(*shipEntity) && ships.has(*shipEntity)) {
        Ship &ship = ships.get(*shipEntity);
        ailmentSystem_.tick(dt, *shipEntity, ship.health, ship.ailments, {ship.x, ship.y}, popupScratch_, appliedScratch_);
    }

    for (const auto &popup: popupScratch_) {
        bus_.publish(popup);
    }
    for (const auto &applied: appliedScratch_) {
        bus_.publish(applied);
    }
}

PowerUpOnKill::PowerUpOnKill(EventBus &bus, GameWorldManager &world, PowerUpManager &manager)
        : bus_(bus), world_(world), manager_(manager) {
    subscriptionId_ = bus_.subscribeDamageApplied([this](const DamageAppliedEvent &event) { onDamage(event); });
}

PowerUpOnKill::~PowerUpOnKill() {
    if (subscriptionId_ != 0) {
        bus_.unsubscribeDamageApplied(subscriptionId_);
    }
}

void PowerUpOnKill::onDamage(const DamageAppliedEvent &event) {
    if (!event.killed) {
        return;
    }

    const ecs::EntityId target = static_cast<ecs::EntityId>(event.target);
    auto &w = world_.world();
    if (!w.registry.alive(target)) {
        return;
    }

    auto &aliens = w.pool<Alien>();

    if (!aliens.has(target)) {
        return;
    }

    world_.destroyEntity(target);
    manager_.spawnPowerUp(event.worldPos);
}

ScoreTracker::ScoreTracker(EventBus &bus, GameWorldManager &world, int &actualScore)
        : bus_(bus), world_(world), actualScore_(actualScore) {
    subscriptionId_ = bus_.subscribeDamageApplied([this](const DamageAppliedEvent &event) { onDamage(event); });
}

ScoreTracker::~ScoreTracker() {
    if (subscriptionId_ != 0) {
        bus_.unsubscribeDamageApplied(subscriptionId_);
    }
}

void ScoreTracker::onDamage(const DamageAppliedEvent &event) {
    const auto shipId = world_.shipEntity();
    if (event.killed && (!shipId.has_value() || static_cast<ecs::EntityId>(event.target) != *shipId)) {
        actualScore_ += 100;
    }
}

GameMechanicsCoordinator::GameMechanicsCoordinator(EventBus &bus,
                                                   GameWorldManager &world,
                                                   PowerUpManager &powerUpManager,
                                                   AilmentSystem &ailmentSystem,
                                                   AilmentRules &ailRules,
                                                   ShieldRules &shieldRules,
                                                   int &actualScore)
        : damageResolver_(bus, world, ailRules, shieldRules),
          ailmentTicker_(bus, world, ailmentSystem),
          powerUpOnKill_(bus, world, powerUpManager),
          scoreTracker_(bus, world, actualScore) {
}

void GameMechanicsCoordinator::update(float dt) {
    ailmentTicker_.update(dt);
}

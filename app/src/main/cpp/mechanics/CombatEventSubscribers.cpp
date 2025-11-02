#include "CombatEventSubscribers.h"

#include <algorithm>
#include <vector>

DamageResolver::DamageResolver(EventBus &bus,
                               Ship &ship,
                               std::span<Alien> aliens,
                               AilmentRules &ailRules,
                               ShieldRules &shieldRules)
        : bus_(bus), ship_(ship), aliens_(aliens), ailRules_(ailRules), shieldRules_(shieldRules) {
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

    if (event.target == ShipEntityId) {
        damageSystem_.apply(event.target,
                            ship_.health,
                            ship_.resistances,
                            nullptr,
                            ship_.ailments,
                            event,
                            applied,
                            popups);
    } else if (event.target < aliens_.size()) {
        Alien &alien = aliens_[event.target];
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
                             Ship &ship,
                             std::span<Alien> aliens,
                             AilmentSystem &ailmentSystem)
        : bus_(bus), ship_(ship), aliens_(aliens), ailmentSystem_(ailmentSystem) {
    popupScratch_.reserve(32);
    appliedScratch_.reserve(16);
}

void AilmentTicker::update(float dt) {
    popupScratch_.clear();
    appliedScratch_.clear();

    for (uint32_t i = 0; i < aliens_.size(); ++i) {
        Alien &alien = aliens_[i];
        if (!alien.active) {
            continue;
        }
        ailmentSystem_.tick(dt, i, alien.health, alien.ailments,
                            {alien.x, alien.y}, popupScratch_, appliedScratch_);
    }

    ailmentSystem_.tick(dt, ShipEntityId, ship_.health, ship_.ailments,
                        {ship_.x, ship_.y}, popupScratch_, appliedScratch_);

    for (const auto &popup: popupScratch_) {
        bus_.publish(popup);
    }
    for (const auto &applied: appliedScratch_) {
        bus_.publish(applied);
    }
}

PowerUpOnKill::PowerUpOnKill(EventBus &bus, std::span<Alien> aliens, PowerUpManager &manager)
        : bus_(bus), aliens_(aliens), manager_(manager) {
    subscriptionId_ = bus_.subscribeDamageApplied([this](const DamageAppliedEvent &event) { onDamage(event); });
}

PowerUpOnKill::~PowerUpOnKill() {
    if (subscriptionId_ != 0) {
        bus_.unsubscribeDamageApplied(subscriptionId_);
    }
}

void PowerUpOnKill::onDamage(const DamageAppliedEvent &event) {
    if (!event.killed || event.target == ShipEntityId || event.target >= aliens_.size()) {
        return;
    }

    Alien &alien = aliens_[event.target];
    if (!alien.active) {
        return;
    }

    alien.active = false;
    manager_.spawnPowerUp(event.worldPos);
}

ScoreTracker::ScoreTracker(EventBus &bus, int &actualScore)
        : bus_(bus), actualScore_(actualScore) {
    subscriptionId_ = bus_.subscribeDamageApplied([this](const DamageAppliedEvent &event) { onDamage(event); });
}

ScoreTracker::~ScoreTracker() {
    if (subscriptionId_ != 0) {
        bus_.unsubscribeDamageApplied(subscriptionId_);
    }
}

void ScoreTracker::onDamage(const DamageAppliedEvent &event) {
    if (event.killed && event.target != ShipEntityId) {
        actualScore_ += 100;
    }
}

GameMechanicsCoordinator::GameMechanicsCoordinator(EventBus &bus,
                                                   Ship &ship,
                                                   std::span<Alien> aliens,
                                                   PowerUpManager &powerUpManager,
                                                   AilmentSystem &ailmentSystem,
                                                   AilmentRules &ailRules,
                                                   ShieldRules &shieldRules,
                                                   int &actualScore)
        : damageResolver_(bus, ship, aliens, ailRules, shieldRules),
          ailmentTicker_(bus, ship, aliens, ailmentSystem),
          powerUpOnKill_(bus, aliens, powerUpManager),
          scoreTracker_(bus, actualScore) {
}

void GameMechanicsCoordinator::update(float dt) {
    ailmentTicker_.update(dt);
}

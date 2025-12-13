#include "SimulationScheduler.h"

#include "ParticleSystem.h"
#include "PowerUpManager.h"
#include "events/EventBus.h"
#include "game/GameWorldManager.h"
#include "mechanics/CombatEventSubscribers.h"
#include "mechanics/Damage.h"

SimulationScheduler::SimulationScheduler(Dependencies deps) : deps_(deps) {
    systems_.emplace_back([this](float dt, bool isPlaying) {
        alienSpawnSystem_.update(deps_.world, dt, isPlaying);
    });

    systems_.emplace_back([this](float dt, bool isPlaying) {
        if (!isPlaying) return;
        const auto settingsId = deps_.world.settingsEntity();
        if (!settingsId.has_value()) return;

        auto &world = deps_.world.world();
        auto &movementPool = world.pool<DirectionalMovement>();
        if (auto *movement = movementPool.tryGet(*settingsId)) {
            alienMovementSystem_.update(world, *movement, dt);
        }
    });

    systems_.emplace_back([this](float dt, bool isPlaying) {
        firingSystem_.update(deps_.world, dt, isPlaying);
    });

    systems_.emplace_back([this](float dt, bool isPlaying) {
        if (!isPlaying) return;
        std::vector<ecs::EntityId> toDestroy;
        toDestroy.reserve(8);
        bulletMovementSystem_.update(deps_.world.world(), dt, toDestroy);
        for (auto e : toDestroy) {
            deps_.world.destroyEntity(e);
        }
    });

    systems_.emplace_back([this](float, bool isPlaying) {
        if (!isPlaying) return;
        collisionSystem_.update(deps_.world, deps_.particles, deps_.events, deps_.powerUps.shieldActive);
    });
}

void SimulationScheduler::setShipInput(float x, float y, bool fireBullet) {
    pendingShipPos_ = {x, y - 0.12f};
    hasPendingInput_ = true;
    wantsToFire_ = wantsToFire_ || fireBullet;
}

void SimulationScheduler::applyShipInput() {
    auto &world = deps_.world.world();
    auto shipId = deps_.world.shipEntity();
    if (!shipId.has_value()) {
        return;
    }
    auto &ships = world.pool<Ship>();
    if (!ships.has(*shipId)) {
        return;
    }
    if (!hasPendingInput_) {
        return;
    }

    auto &ship = ships.get(*shipId);
    ship.x = pendingShipPos_.x;
    ship.y = pendingShipPos_.y;

    hasPendingInput_ = false;
}

void SimulationScheduler::trySpawnShipBullet() {
    if (fireAccumulator_ < rateOfFire_) {
        return;
    }
    if (!wantsToFire_) {
        return;
    }

    auto &world = deps_.world.world();
    const auto shipEntity = deps_.world.shipEntity();
    if (!shipEntity.has_value()) {
        return;
    }

    auto &ships = world.pool<Ship>();
    if (!ships.has(*shipEntity)) {
        return;
    }

    const bool doubleShot = deps_.powerUps.doubleShotActive;
    const glm::vec2 spawnPos{pendingShipPos_.x, pendingShipPos_.y};
    if (doubleShot) {
        (void) deps_.world.spawnBullet(dualBulletPrefab_, {spawnPos.x - 0.05f, spawnPos.y - 0.04f});
        (void) deps_.world.spawnBullet(dualBulletPrefab_, {spawnPos.x + 0.05f, spawnPos.y - 0.04f});
    } else {
        (void) deps_.world.spawnBullet(shipBulletPrefab_, {spawnPos.x, spawnPos.y - 0.04f});
    }

    fireAccumulator_ = 0.0f;
    wantsToFire_ = false;
}

void SimulationScheduler::tick(float dt, bool isPlaying) {
    fireAccumulator_ += dt;

    deps_.world.decayFlash(dt);

    if (!isPlaying) {
        wantsToFire_ = false;
        hasPendingInput_ = false;
        return;
    }

    applyShipInput();
    trySpawnShipBullet();

    for (auto &system : systems_) {
        system(dt, isPlaying);
    }
    if (deps_.mechanics) {
        deps_.mechanics->update(dt);
    }
    deps_.powerUps.updatePowerUpData();
    const auto shipId = deps_.world.shipEntity();
    if (shipId.has_value()) {
        auto &ships = deps_.world.world().pool<Ship>();
        if (ships.has(*shipId)) {
            deps_.powerUps.checkIfPowerUpCollected(ships.get(*shipId));
        }
    }
}

void SimulationScheduler::resetWorld() {
    deps_.world.initAliens();
    const auto shipId = deps_.world.shipEntity();
    if (shipId.has_value()) {
        auto &ships = deps_.world.world().pool<Ship>();
        if (ships.has(*shipId)) {
            ships.get(*shipId).health.hull = 100.0f;
        }
    }

    fireAccumulator_ = rateOfFire_;
    wantsToFire_ = false;
    hasPendingInput_ = false;
}


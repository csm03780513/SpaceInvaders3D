#include "SimulationScheduler.h"

#include "ParticleSystem.h"
#include "PowerUpManager.h"
#include "events/EventBus.h"
#include "game/GameWorldManager.h"
#include "mechanics/CombatEventSubscribers.h"
#include "mechanics/Damage.h"

SimulationScheduler::SimulationScheduler(Dependencies deps) : deps_(deps) {
    systems_.emplace_back([this](float dt, bool isPlaying) {
        shipInputSystem_.update(deps_.world, deps_.powerUps, dt, isPlaying);
    });

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

        std::vector<ecs::EntityId> toDestroy;
        toDestroy.reserve(8);
        bulletMovementSystem_.update(world, dt, toDestroy);
        for (auto e : toDestroy) {
            deps_.world.destroyEntity(e);
        }
    });

    systems_.emplace_back([this](float dt, bool isPlaying) {
        firingSystem_.update(deps_.world, dt, isPlaying);
        bulletSpawnSystem_.update(deps_.world, isPlaying);
    });

    systems_.emplace_back([this](float, bool isPlaying) {
        if (!isPlaying) return;
        collisionSystem_.update(deps_.world, deps_.particles, deps_.events, deps_.powerUps.shieldActive);
    });

    systems_.emplace_back([this](float dt, bool isPlaying) {
        if (!isPlaying) return;
        if (deps_.mechanics) {
            deps_.mechanics->update(dt);
        }
    });
}

void SimulationScheduler::setShipInput(float x, float y, bool fireBullet) {
    shipInputSystem_.queueInput({x, y - 0.12f}, fireBullet);
}

void SimulationScheduler::tick(float dt, bool isPlaying) {
    deps_.world.decayFlash(dt);

    for (auto &system : systems_) {
        system(dt, isPlaying);
    }

    if (!isPlaying) {
        return;
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

    shipInputSystem_.reset();
}


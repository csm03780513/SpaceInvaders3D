#include "SimulationScheduler.h"

#include "ParticleSystem.h"
#include "PowerUpManager.h"
#include "events/EventBus.h"
#include "game/GameWorldManager.h"
#include "mechanics/CombatEventSubscribers.h"
#include "mechanics/Damage.h"
#include "Util.h"

SimulationScheduler::SimulationScheduler(Dependencies deps) : deps_(deps) {}

void SimulationScheduler::setShipInput(float x, float y, bool fireBullet) {
    pendingShipPos_ = {x, y - 0.12f};
    hasPendingInput_ = true;
    wantsToFire_ = wantsToFire_ || fireBullet;
}

void SimulationScheduler::applyShipInput() {
    auto &world = deps_.world.world();
    if (!world.ships.has(deps_.world.shipEntity())) {
        return;
    }
    if (!hasPendingInput_) {
        return;
    }

    auto &ship = world.ships.get(deps_.world.shipEntity());
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
    if (!world.ships.has(shipEntity)) {
        return;
    }

    const bool doubleShot = deps_.powerUps.doubleShotActive;
    const glm::vec2 spawnPos{pendingShipPos_.x, pendingShipPos_.y};
    if (doubleShot) {
        (void) deps_.world.spawnBullet(BulletType::Ship,
                                       {spawnPos.x - 0.05f, spawnPos.y - 0.04f},
                                       makeKinetic(Util::getRandomFloat(10.0f, 40.0f), 0.2f));
        (void) deps_.world.spawnBullet(BulletType::Ship,
                                       {spawnPos.x + 0.05f, spawnPos.y - 0.04f},
                                       makeKinetic(Util::getRandomFloat(10.0f, 40.0f)));
    } else {
        (void) deps_.world.spawnBullet(BulletType::Ship,
                                       {spawnPos.x, spawnPos.y - 0.04f},
                                       makePlasma(Util::getRandomFloat(10.0f, 40.0f)));
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

    deps_.world.setBulletSpeeds(shipBulletSpeed_, alienBulletSpeed_);
    applyShipInput();
    trySpawnShipBullet();

    deps_.world.updateAliens(dt);
    deps_.world.updateBullets(dt);
    deps_.world.updateAndMaybeFire(isPlaying, dt);

    deps_.world.processCollisions(deps_.powerUps.shieldActive, deps_.particles, deps_.events);
    if (deps_.mechanics) {
        deps_.mechanics->update(dt);
    }
    deps_.powerUps.updatePowerUpData();
    deps_.powerUps.checkIfPowerUpCollected(deps_.world.world().ships.get(deps_.world.shipEntity()));
}

void SimulationScheduler::resetWorld() {
    deps_.world.initAliens();
    deps_.world.world().ships.get(deps_.world.shipEntity()).health.hull = 100.0f;

    fireAccumulator_ = rateOfFire_;
    wantsToFire_ = false;
    hasPendingInput_ = false;
}


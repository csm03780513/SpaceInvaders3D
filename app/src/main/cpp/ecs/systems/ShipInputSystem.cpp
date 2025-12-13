#include "ShipInputSystem.h"

#include "ecs/components/CombatComponents.h"
#include "ecs/components/GameplayComponents.h"
#include "game/GameWorldManager.h"
#include "PowerUpManager.h"

namespace ecs {

void ShipInputSystem::queueInput(const glm::vec2 &pos, bool wantsToFire) {
    pendingShipPos_ = pos;
    hasPendingInput_ = true;
    wantsToFire_ = wantsToFire_ || wantsToFire;
}

void ShipInputSystem::applyPendingInput(GameWorldManager &manager) {
    auto &world = manager.world();
    auto shipId = manager.shipEntity();
    if (!shipId.has_value()) {
        return;
    }

    auto &ships = world.pool<Ship>();
    if (!ships.has(*shipId) || !hasPendingInput_) {
        return;
    }

    auto &ship = ships.get(*shipId);
    ship.x = pendingShipPos_.x;
    ship.y = pendingShipPos_.y;

    hasPendingInput_ = false;
}

void ShipInputSystem::queueBulletRequests(GameWorldManager &manager, PowerUpManager &powerUps) {
    auto settingsId = manager.settingsEntity();
    if (!settingsId.has_value()) {
        return;
    }

    auto &world = manager.world();
    auto &spawns = world.pool<SpawnRequests>();
    if (!spawns.has(*settingsId)) {
        spawns.add(*settingsId, SpawnRequests{});
    }

    auto &queue = spawns.get(*settingsId);

    const bool doubleShot = powerUps.doubleShotActive;
    const glm::vec2 spawnPos{pendingShipPos_.x, pendingShipPos_.y};
    if (doubleShot) {
        queue.bullets.push_back(SpawnBulletRequest{dualBulletPrefab_, {spawnPos.x - 0.05f, spawnPos.y - 0.04f}});
        queue.bullets.push_back(SpawnBulletRequest{dualBulletPrefab_, {spawnPos.x + 0.05f, spawnPos.y - 0.04f}});
    } else {
        queue.bullets.push_back(SpawnBulletRequest{shipBulletPrefab_, {spawnPos.x, spawnPos.y - 0.04f}});
    }
}

void ShipInputSystem::update(GameWorldManager &manager, PowerUpManager &powerUps, float deltaTime, bool isPlaying) {
    fireAccumulator_ += deltaTime;

    if (!isPlaying) {
        wantsToFire_ = false;
        hasPendingInput_ = false;
        return;
    }

    applyPendingInput(manager);

    if (fireAccumulator_ < rateOfFire_ || !wantsToFire_) {
        return;
    }

    queueBulletRequests(manager, powerUps);

    fireAccumulator_ = 0.0f;
    wantsToFire_ = false;
}

void ShipInputSystem::reset() {
    fireAccumulator_ = rateOfFire_;
    wantsToFire_ = false;
    hasPendingInput_ = false;
}

} // namespace ecs


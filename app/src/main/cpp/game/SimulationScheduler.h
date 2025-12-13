#pragma once

#include <functional>
#include <glm/vec2.hpp>
#include <memory>
#include <string>
#include <vector>

#include "GameConstants.h"
#include "ecs/components/GameplayComponents.h"
#include "ecs/systems/AlienMovementSystem.h"
#include "ecs/systems/AlienSpawnSystem.h"
#include "ecs/systems/BulletMovementSystem.h"
#include "ecs/systems/FiringSystem.h"

class EventBus;
class GameMechanicsCoordinator;
class GameWorldManager;
class ParticleSystem;
class PowerUpManager;

class SimulationScheduler {
public:
    struct Dependencies {
        GameWorldManager &world;
        PowerUpManager &powerUps;
        EventBus &events;
        ParticleSystem &particles;
        GameMechanicsCoordinator *mechanics;
    };

    explicit SimulationScheduler(Dependencies deps);

    void tick(float dt, bool isPlaying);
    void setShipInput(float x, float y, bool fireBullet);
    void resetWorld();

private:
    void applyShipInput();
    void trySpawnShipBullet();

    Dependencies deps_;
    glm::vec2 pendingShipPos_{0.0f};
    bool hasPendingInput_{false};
    bool wantsToFire_{false};

    float rateOfFire_ = 0.2f;
    float fireAccumulator_ = 0.0f;
    std::string shipBulletPrefab_{"ship_primary"};
    std::string dualBulletPrefab_{"ship_dual"};

    ecs::AlienSpawnSystem alienSpawnSystem_{};
    ecs::AlienMovementSystem alienMovementSystem_{};
    ecs::FiringSystem firingSystem_{};
    ecs::BulletMovementSystem bulletMovementSystem_{};
    std::vector<std::function<void(float, bool)>> systems_{};
};


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
#include "ecs/systems/BulletSpawnSystem.h"
#include "ecs/systems/BulletMovementSystem.h"
#include "ecs/systems/CollisionSystem.h"
#include "ecs/systems/FiringSystem.h"
#include "ecs/systems/ShipInputSystem.h"

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
    Dependencies deps_;

    ecs::ShipInputSystem shipInputSystem_{};
    ecs::AlienSpawnSystem alienSpawnSystem_{};
    ecs::AlienMovementSystem alienMovementSystem_{};
    ecs::BulletSpawnSystem bulletSpawnSystem_{};
    ecs::FiringSystem firingSystem_{};
    ecs::BulletMovementSystem bulletMovementSystem_{};
    ecs::CollisionSystem collisionSystem_{};
    std::vector<std::function<void(float, bool)>> systems_{};
};


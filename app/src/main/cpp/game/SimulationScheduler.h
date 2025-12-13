#pragma once

#include <glm/vec2.hpp>
#include <memory>
#include <string>

#include "GameConstants.h"

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
};


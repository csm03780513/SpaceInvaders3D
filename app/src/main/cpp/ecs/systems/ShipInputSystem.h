#pragma once

#include <glm/vec2.hpp>
#include <string>

class GameWorldManager;
class PowerUpManager;

namespace ecs {

class ShipInputSystem {
public:
    void queueInput(const glm::vec2 &pos, bool wantsToFire);

    void update(GameWorldManager &manager, PowerUpManager &powerUps, float deltaTime, bool isPlaying);

    void reset();

private:
    void applyPendingInput(GameWorldManager &manager);
    void queueBulletRequests(GameWorldManager &manager, PowerUpManager &powerUps);

    glm::vec2 pendingShipPos_{0.0f};
    bool hasPendingInput_{false};
    bool wantsToFire_{false};

    float rateOfFire_ = 0.2f;
    float fireAccumulator_ = 0.0f;
    std::string shipBulletPrefab_{"ship_primary"};
    std::string dualBulletPrefab_{"ship_dual"};
};

} // namespace ecs


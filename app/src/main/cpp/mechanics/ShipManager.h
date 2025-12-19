#pragma once

#include <memory>

#include "../GameObjectData.h"
#include "../ecs/components/CombatComponents.h"

class Util;
class ProjectileManager;
class PowerUpManager;
class SFXMixer;

class ShipManager {
public:
    ShipManager() = default;

    Ship &ship();
    const Ship &ship() const;

    MainPushConstants &pushConstants();
    const MainPushConstants &pushConstants() const;

    void initialize(const std::shared_ptr<Util> &util, const std::shared_ptr<SFXMixer> &sfxMixer);
    void setInput(float x, float y, bool fireBullet,
                  ProjectileManager *projectiles, PowerUpManager *powerUps);
    void update(float dt);
    void resetForNewGame(bool alienBelow);
    bool isDead() const;

private:
    Ship ship_{};
    MainPushConstants shipPC_{.texturePos = 0};
    float rateOfFire_ = 0.2f;
    float lastFireTime_ = 0.0f;
    bool canFire_ = false;
    SFXMixer *sfxMixer_ = nullptr;
};

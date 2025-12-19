#pragma once

#include <array>
#include <span>
#include <glm/vec2.hpp>

#include "../GameObjectData.h"
#include "../Collision.h"
#include "../Util.h"
#include "../events/EventBus.h"
#include "../ecs/components/CombatComponents.h"
#include "../ecs/events/CombatEvents.h"
#include "Damage.h"
#include "AlienManager.h"

class AlienManager;

class ParticleSystem;

class PowerUpManager;

struct Bullet {
    float x{}, y{};
    bool active{};
    float moveSpeed{2.0f};
    glm::vec2 velocity{0.0f, 0.5f};
    std::array<float, 2> widthHeight{};
    BulletType bulletType{};
    const float size = 0.05f * 0.5f; //half alien
    DamagePayload payload{};
};

// Owns bullet lifecycle: spawn, movement, collisions, and push constants.
class ProjectileManager {
public:
    ProjectileManager(EventBus &eventBus, PowerUpManager &powerUps, ParticleSystem &particles,
                      std::shared_ptr<SFXMixer>  sfxMixer);

    void setBulletSize(const std::array<float, 2> &size);

    void reset();

    void spawnShipBullets(glm::vec2 spawnPos, bool doubleShot, bool canFire);

    void spawnAlienBullet(const Alien &alien, const Ship &ship);

    void update(float dt);

    void handleCollisions(std::span<Alien> aliens, Ship &ship, MainPushConstants &shipPC,
                          AlienManager &alienManager);

    void tryAlienFire(float dt, AlienManager &alienManager, const Ship &ship);

    std::span<Bullet> bullets();

    std::span<MainPushConstants> pushConstants();

private:
    bool isCollision(const Alien &alien, const Bullet &bullet, const Ship &ship) const;

    EventBus &eventBus_;
    PowerUpManager &powerUpManager_;
    ParticleSystem &particleSystem_;
    std::shared_ptr<SFXMixer> sfxMixer_;

    std::array<Bullet, MAX_BULLETS> bullets_{};
    std::array<MainPushConstants, MAX_BULLETS> bulletPC_{};
    std::array<float, 2> bulletSize_{};

    float bulletMoveSpeed_ = 2.0f;
    float alienFireTimer_ = 0.0f;

};

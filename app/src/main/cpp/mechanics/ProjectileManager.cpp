#include "ProjectileManager.h"
#include "AlienManager.h"
#include "../ParticleSystem.h"
#include "../PowerUpManager.h"
#include <glm/vec3.hpp>

ProjectileManager::ProjectileManager(EventBus &eventBus, PowerUpManager &powerUps, ParticleSystem &particles)
        : eventBus_(eventBus), powerUpManager_(powerUps), particleSystem_(particles) {}

void ProjectileManager::setBulletSize(const std::array<float, 2> &size) {
    bulletSize_ = size;
    for (auto &bullet: bullets_) {
        bullet.widthHeight = size;
    }
}

void ProjectileManager::reset() {
    for (auto &bullet: bullets_) {
        bullet.active = false;
    }
}

void ProjectileManager::spawnShipBullets(glm::vec2 spawnPos, bool doubleShot, bool canFire) {
    if (!canFire) return;

    int spawned = 0;
    for (auto &bullet: bullets_) {
        if (bullet.active) continue;
        bullet.bulletType = BulletType::Ship;

        if (doubleShot && spawned == 0) {
            bullet.x = spawnPos.x - 0.05f;
            bullet.y = spawnPos.y - 0.04f;
            bullet.active = true;
            bullet.payload = makeKinetic(Util::getRandomFloat(10.0f, 40.0f), 0.2f);
            spawned++;
            continue;
        }

        if (doubleShot && spawned == 1) {
            bullet.x = spawnPos.x + 0.05f;
            bullet.y = spawnPos.y - 0.04f;
            bullet.active = true;
            bullet.payload = makeKinetic(Util::getRandomFloat(10.0f, 40.0f));
            break;
        }

        if (!doubleShot) {
            bullet.x = spawnPos.x;
            bullet.y = spawnPos.y - 0.04f;
            bullet.active = true;
            bullet.payload = makePlasma(Util::getRandomFloat(10.0f, 40.0f));
            break;
        }
    }
}

void ProjectileManager::spawnAlienBullet(glm::vec2 spawnPos) {
    for (auto &bullet: bullets_) {
        if (bullet.active) continue;
        bullet.x = spawnPos.x;
        bullet.y = spawnPos.y + 0.04f;
        bullet.active = true;
        bullet.bulletType = BulletType::Alien;
        bullet.payload = makeKinetic(Util::getRandomFloat(10.0f, 30.0f));
        break;
    }
}

void ProjectileManager::update(float dt) {
    for (auto &bullet: bullets_) {
        if (!bullet.active) continue;

        if (bullet.bulletType == BulletType::Ship) {
            bullet.y -= bulletMoveSpeed_ * dt;
        } else if (bullet.bulletType == BulletType::Alien) {
            bullet.y += 0.5f * dt;
        }

        if ((bullet.bulletType == BulletType::Ship && bullet.y < -1.0f) ||
            (bullet.bulletType == BulletType::Alien && bullet.y > 1.0f)) {
            bullet.active = false;
        }
    }
}

bool ProjectileManager::isCollision(const Alien &alien, const Bullet &bullet, const Ship &ship) const {
    if (bullet.bulletType == BulletType::Ship) {
        auto alienAABB = Collision::getAABB(alien.x, alien.y, alien.widthHeight[0], alien.widthHeight[1]);
        auto bulletAABB = Collision::getAABB(bullet.x, -bullet.y, bullet.widthHeight[0], bullet.widthHeight[1]);
        return Collision::isColliding(alienAABB, bulletAABB);
    }

    if (bullet.bulletType == BulletType::Alien) {
        auto shipAABB = Collision::getAABB(ship.x, ship.y, ship.widthHeight[0], ship.widthHeight[1]);
        auto bulletAABB = Collision::getAABB(bullet.x, bullet.y, bullet.widthHeight[0], bullet.widthHeight[1]);
        return Collision::isColliding(shipAABB, bulletAABB);
    }

    return false;
}

void ProjectileManager::handleCollisions(std::span<Alien> aliens, Ship &ship, MainPushConstants &shipPC,
                                         AlienManager &alienManager) {
    for (auto &bullet: bullets_) {
        if (!bullet.active) continue;

        for (uint32_t i = 0; i < aliens.size(); ++i) {
            if (!aliens[i].active) continue;

            if (bullet.bulletType == BulletType::Ship && isCollision(aliens[i], bullet, ship)) {
                bullet.active = false;

                eventBus_.publish(HitEvent{
                        .attacker = 0,
                        .target = i,
                        .payload = bullet.payload,
                        .hitWorldPos = glm::vec2(aliens[i].x, aliens[i].y)
                });

                alienManager.flashAlien(i);
                particleSystem_.spawn(glm::vec3(aliens[i].x, -aliens[i].y, 1.0f), 5);
                break;
            }

            if (bullet.bulletType == BulletType::Alien &&
                !powerUpManager_.shieldActive &&
                isCollision(aliens[i], bullet, ship)) {

                bullet.active = false;

                eventBus_.publish(HitEvent{
                        .attacker = i,
                        .target = ShipEntityId,
                        .payload = bullet.payload,
                        .hitWorldPos = glm::vec2(ship.x, -ship.y)
                });

                shipPC.flashAmount = 1.0f;
                particleSystem_.spawn(glm::vec3(bullet.x, bullet.y, 0.0f), 10);
                break;
            }
        }
    }
}

void ProjectileManager::tryAlienFire(float dt, AlienManager &alienManager) {
    alienFireTimer_ += dt;
    if (alienFireTimer_ < 1.0f) return;

    alienFireTimer_ = 0.0f;
    auto randomAlien = alienManager.randomActiveAlienPos();
    if (randomAlien.has_value()) {
        spawnAlienBullet({randomAlien->x, -randomAlien->y});
    }
}

std::span<Bullet> ProjectileManager::bullets() {
    return bullets_;
}

std::span<MainPushConstants> ProjectileManager::pushConstants() {
    return bulletPC_;
}

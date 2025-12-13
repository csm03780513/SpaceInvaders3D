#include "CollisionSystem.h"

#include <utility>
#include <vector>

#include "Collision.h"
#include "ParticleSystem.h"
#include "ecs/components/CombatComponents.h"
#include "ecs/components/GameplayComponents.h"
#include "events/EventBus.h"
#include "game/GameWorldManager.h"

namespace {

bool isShipBulletHittingAlien(const Alien &alien, const Bullet &bullet) {
    const auto alienAABB = Collision::getAABB(alien.x, alien.y, alien.widthHeight[0], alien.widthHeight[1]);
    const auto bulletAABB = Collision::getAABB(bullet.x, -bullet.y, bullet.widthHeight[0], bullet.widthHeight[1]);
    return Collision::isColliding(alienAABB, bulletAABB);
}

bool isAlienBulletHittingShip(const Ship &ship, const Bullet &bullet) {
    const auto shipAABB = Collision::getAABB(ship.x, ship.y, ship.widthHeight[0], ship.widthHeight[1]);
    const auto bulletAABB = Collision::getAABB(bullet.x, bullet.y, bullet.widthHeight[0], bullet.widthHeight[1]);
    return Collision::isColliding(shipAABB, bulletAABB);
}

} // namespace

namespace ecs {

void CollisionSystem::update(GameWorldManager &manager,
                             ParticleSystem &particleSystem,
                             EventBus &eventBus,
                             bool shieldActive) const {
    const auto shipId = manager.shipEntity();
    auto &world = manager.world();
    if (!shipId.has_value() || !world.registry.alive(*shipId)) {
        return;
    }

    auto &ships = world.pool<Ship>();
    auto &aliens = world.pool<Alien>();
    auto &bullets = world.pool<Bullet>();
    auto &render = world.pool<MainPushConstants>();

    Ship *ship = ships.tryGet(*shipId);
    if (!ship) {
        return;
    }

    std::vector<EntityId> bulletsToDestroy;
    bulletsToDestroy.reserve(8);

    std::vector<std::pair<EntityId, Alien *>> activeAliens;
    world.registry.forEachAlive([&](EntityId e) {
        Alien *alien = aliens.tryGet(e);
        if (alien && alien->active) {
            activeAliens.emplace_back(e, alien);
        }
    });

    world.registry.forEachAlive([&](EntityId bulletEntity) {
        Bullet *bullet = bullets.tryGet(bulletEntity);
        if (!bullet || !bullet->active) {
            return;
        }

        if (bullet->bulletType == BulletType::Ship) {
            for (auto &[alienEntity, alien] : activeAliens) {
                if (!isShipBulletHittingAlien(*alien, *bullet)) {
                    continue;
                }

                bullet->active = false;
                bulletsToDestroy.push_back(bulletEntity);

                eventBus.publish(HitEvent{
                        .attacker = *shipId,
                        .target = alienEntity,
                        .payload = bullet->payload,
                        .hitWorldPos = glm::vec2(alien->x, alien->y),
                });

                if (render.has(alienEntity)) {
                    render.get(alienEntity).flashAmount = 1.0f;
                }
                particleSystem.spawn(glm::vec3(alien->x, -alien->y, 1.0f), 5);
                break;
            }
            return;
        }

        if (bullet->bulletType == BulletType::Alien && !shieldActive) {
            if (isAlienBulletHittingShip(*ship, *bullet)) {
                bullet->active = false;
                bulletsToDestroy.push_back(bulletEntity);
                eventBus.publish(HitEvent{
                        .attacker = 0,
                        .target = *shipId,
                        .payload = bullet->payload,
                        .hitWorldPos = glm::vec2(ship->x, ship->y),
                });

                if (render.has(*shipId)) {
                    render.get(*shipId).flashAmount = 1.0f;
                }
                particleSystem.spawn(glm::vec3(bullet->x, bullet->y, 0.0f), 10);
            }
        }
    });

    for (auto bulletEntity : bulletsToDestroy) {
        manager.destroyEntity(bulletEntity);
    }
}

} // namespace ecs


#pragma once

#include <algorithm>

#include "GameConstants.h"
#include "ecs/worlds/CombatWorld.h"

namespace ecs {

class BulletMovementSystem {
public:
    void update(CombatWorld &world, float deltaTime, float shipBulletSpeed, float alienBulletSpeed) {
        world.bulletEntities.forEachAlive([&](EntityId id) {
            Bullet &bullet = world.bullets[id];
            if (!bullet.active) {
                world.bulletEntities.destroy(id);
                return;
            }

            if (bullet.bulletType == BulletType::Ship) {
                bullet.y -= shipBulletSpeed * deltaTime;
            } else if (bullet.bulletType == BulletType::Alien) {
                bullet.y += alienBulletSpeed * deltaTime;
            }

            if ((bullet.bulletType == BulletType::Ship && bullet.y < -1.0f) ||
                (bullet.bulletType == BulletType::Alien && bullet.y > 1.0f)) {
                bullet.active = false;
                world.bulletEntities.destroy(id);
            }
        });
    }
};

} // namespace ecs


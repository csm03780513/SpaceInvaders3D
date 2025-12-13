#pragma once

#include <vector>

#include "GameConstants.h"
#include "ecs/worlds/GameWorld.h"

namespace ecs {

class BulletMovementSystem {
public:
    void update(GameWorld &world, float deltaTime, std::vector<EntityId> &toDestroy) const {
        world.registry.forEachAlive([&](EntityId id) {
            Bullet *bullet = world.bullets.tryGet(id);
            if (!bullet) return;

            if (!bullet->active) {
                toDestroy.push_back(id);
                return;
            }

            if (bullet->bulletType == BulletType::Ship) {
                bullet->y -= bullet->speed * deltaTime;
            } else if (bullet->bulletType == BulletType::Alien) {
                bullet->y += bullet->speed * deltaTime;
            }

            if ((bullet->bulletType == BulletType::Ship && bullet->y < -1.0f) ||
                (bullet->bulletType == BulletType::Alien && bullet->y > 1.0f)) {
                bullet->active = false;
                toDestroy.push_back(id);
            }
        });
    }
};

} // namespace ecs


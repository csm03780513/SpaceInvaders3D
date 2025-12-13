#include "BulletSpawnSystem.h"

#include "ecs/components/GameplayComponents.h"
#include "game/GameWorldManager.h"

namespace ecs {

void BulletSpawnSystem::update(GameWorldManager &manager, bool isPlaying) const {
    if (!isPlaying) {
        return;
    }

    auto settingsId = manager.settingsEntity();
    if (!settingsId.has_value()) {
        return;
    }

    auto &world = manager.world();
    auto &spawns = world.pool<SpawnRequests>();
    auto *queue = spawns.tryGet(*settingsId);
    if (!queue || queue->bullets.empty()) {
        return;
    }

    for (const auto &spawn : queue->bullets) {
        (void) manager.spawnBullet(spawn.prefab, spawn.position);
    }

    queue->bullets.clear();
}

} // namespace ecs


#include "FiringSystem.h"

#include <vector>

#include "Util.h"
#include "ecs/components/CombatComponents.h"
#include "ecs/components/GameplayComponents.h"
#include "ecs/worlds/GameWorld.h"
#include "game/GameWorldManager.h"

namespace ecs {

void FiringSystem::update(GameWorldManager &manager, float deltaTime, bool isPlaying) {
    if (!isPlaying) return;

    const auto settingsId = manager.settingsEntity();
    if (!settingsId.has_value()) return;

    auto &world = manager.world();
    auto &cooldowns = world.pool<FireCooldown>();
    auto *cooldown = cooldowns.tryGet(*settingsId);
    if (!cooldown) return;

    cooldown->timer += deltaTime;
    if (cooldown->timer <= cooldown->interval) return;
    cooldown->timer = 0.0f;

    auto &wavePool = world.pool<WaveSettings>();
    auto *settings = wavePool.tryGet(*settingsId);

    std::vector<EntityId> aliveAliens;
    aliveAliens.reserve(8);
    auto &aliens = world.pool<Alien>();
    world.registry.forEachAlive([&](EntityId e) {
        const Alien *alien = aliens.tryGet(e);
        if (alien && alien->active) aliveAliens.push_back(e);
    });
    if (aliveAliens.empty()) return;

    const uint32_t idx = Util::getRandomUint(0, static_cast<uint32_t>(aliveAliens.size() - 1));
    const Alien &alien = aliens.get(aliveAliens[idx]);
    const std::string bulletPrefab = settings ? settings->activeAlienBulletPrefab : std::string{"alien_primary"};
    (void) manager.spawnBullet(bulletPrefab, {alien.x, -alien.y});
}

} // namespace ecs


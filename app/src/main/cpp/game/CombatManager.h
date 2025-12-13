#pragma once

#include <optional>
#include <span>

#include <glm/vec2.hpp>

#include "GameConstants.h"
#include "ecs/components/CombatComponents.h"
#include "ecs/systems/BulletMovementSystem.h"
#include "ecs/worlds/CombatWorld.h"

class CombatManager {
public:
    CombatManager() = default;

    [[nodiscard]] Ship &ship();
    [[nodiscard]] const Ship &ship() const;

    [[nodiscard]] std::span<Bullet> bullets();
    [[nodiscard]] std::span<const Bullet> bullets() const;

    void resetBullets();
    void updateBullets(float deltaTime, float shipBulletSpeed, float alienBulletSpeed);

    // Spawns a single bullet entity; returns its id (0..MAX_BULLETS-1) or nullopt if pool is full.
    std::optional<uint32_t> spawnBullet(BulletType type, const glm::vec2 &spawnPos);
    void deactivateBullet(uint32_t id);

private:
    ecs::CombatWorld world_{};
    ecs::BulletMovementSystem bulletMovementSystem_{};
};


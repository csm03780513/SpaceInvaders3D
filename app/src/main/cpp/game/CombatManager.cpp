#include "CombatManager.h"

Ship &CombatManager::ship() {
    return world_.ship;
}

const Ship &CombatManager::ship() const {
    return world_.ship;
}

std::span<Bullet> CombatManager::bullets() {
    return {world_.bullets.data(), world_.bullets.size()};
}

std::span<const Bullet> CombatManager::bullets() const {
    return {world_.bullets.data(), world_.bullets.size()};
}

void CombatManager::resetBullets() {
    world_.bulletEntities.clear();
    for (auto &b : world_.bullets) {
        b.active = false;
    }
}

void CombatManager::updateBullets(float deltaTime, float shipBulletSpeed, float alienBulletSpeed) {
    bulletMovementSystem_.update(world_, deltaTime, shipBulletSpeed, alienBulletSpeed);
}

std::optional<uint32_t> CombatManager::spawnBullet(BulletType type, const glm::vec2 &spawnPos) {
    auto idOpt = world_.bulletEntities.create();
    if (!idOpt.has_value()) {
        return std::nullopt;
    }

    const uint32_t id = *idOpt;
    Bullet &b = world_.bullets[id];
    b.active = true;
    b.bulletType = type;
    b.x = spawnPos.x;
    b.y = spawnPos.y;
    return id;
}

void CombatManager::deactivateBullet(uint32_t id) {
    if (id >= MAX_BULLETS) return;
    world_.bullets[id].active = false;
    world_.bulletEntities.destroy(id);
}


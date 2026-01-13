#include "AlienManager.h"

#include <algorithm>
#include <cmath>

#include "Util.h"


AlienManager::AlienManager(std::shared_ptr<PowerUpManager> powerUpManager)
        : powerUpManager_(std::move(powerUpManager)) {
}

void AlienManager::resetMovementState() {
//    alienMoveSpeed_ = 0.3f;
    alienDirection_ = 1.0f;
}

void AlienManager::initAliens() {
    resetMovementState();

    Resistances enemyRes{};
    enemyRes.byType[(int) DamageType::Kinetic] = 0.10f;
    enemyRes.byType[(int) DamageType::Fire] = 0.10f;
    enemyRes.byType[(int) DamageType::Lightning] = 0.05f;
    enemyRes.byType[(int) DamageType::Cold] = 0.00f;
    enemyRes.byType[(int) DamageType::Poison] = 0.00f;
    enemyRes.byType[(int) DamageType::Radiation] = 0.15f;
    enemyRes.byType[(int) DamageType::Plasma] = 0.05f;
    enemyRes.byType[(int) DamageType::DarkMatter] = -0.10f; // vulnerable
    enemyRes.byType[(int) DamageType::Cosmic] = 0.20f;

    float startX = -0.7f;
    float startY = 0.8f;
    float dx = 0.2f;
    float dy = 0.15f;
    level_++;
    if (powerUpManager_) {
        powerUpManager_->powerUpChance += 0.05f;
    }
    if(wave_ == 3) {
        buildBossAlien();
        wave_++;
        return;
    }

    if (wave_ >= 4) wave_ = 0; // reset wave


    for (int y = 0; y < NUM_ALIENS_Y; ++y) {
        for (int x = 0; x < NUM_ALIENS_X; ++x) {
            int idx = y * NUM_ALIENS_X + x;
            aliens_[idx].x = startX + x * dx;
            aliens_[idx].unitType = UnitType::Standard;
            aliens_[idx].baseX = startX + x * dx;
            aliens_[idx].movementTimer = 0.0f;
            aliens_[idx].movementSpeed = 0.3f;
            aliens_[idx].y = startY - y * dy;
            aliens_[idx].active = true;
            aliens_[idx].health.hull = 100.0f;
            aliens_[idx].health.dead = false;
            aliens_[idx].resistances = enemyRes;
            aliens_[idx].ailments = {};

            aliens_[idx].widthHeight = Util::getQuadWidthHeight(quadVerts, 6, {1.0, 1.0});
            alienPC_[idx].texturePos = 1;
            if (wave_ == 0) {
                aliens_[idx].movementType = AlienMovementType::LeftRight;
            } else if (wave_ == 1) {
                aliens_[idx].frequency = aliens_[idx].baseFrequency * 7 + (level_ * 0.05f);
                aliens_[idx].movementType = AlienMovementType::SnakeWave;
                aliens_[idx].vy += 0.01f;
            } else if (wave_ == 2) {
                aliens_[idx].movementType = AlienMovementType::SineWave;
                aliens_[idx].frequency = aliens_[idx].baseFrequency * 5 + (level_ * 0.05f);
                aliens_[idx].vy += 0.01f;
            } else if (wave_ == 3) {
                aliens_[idx].movementType = AlienMovementType::TogetherOne;
                aliens_[idx].x = 0.0f;
                aliens_[idx].vy += 0.01f;
            } else {
                aliens_[idx].movementType = AlienMovementType::JustGoDown;
            }
        }
    }
    wave_++;
}

void AlienManager::update(float dt) {
    bool hitEdge = false;

    for (int i = 0; i < MAX_ALIENS; ++i) {
        auto &alien = aliens_[i];
        if (!alien.active) continue;

        auto &pushConst = alienPC_[i];
        pushConst.flashAmount = std::max(0.0f, pushConst.flashAmount - dt * 5.0f);

        switch (alien.movementType) {
            case TogetherOne:
                alien.y -= alien.vy * dt;
                break;
            case SineWave:
                alien.movementTimer += dt;
                alien.x = alien.baseX + alien.amplitude * std::sin(alien.movementTimer * alien.frequency);
                alien.y -= alien.vy * dt;
                break;
            case MySnakeWave:
                alien.movementTimer += dt;
                alien.x = std::sin((alien.movementTimer + alien.baseX) * alien.frequency);
                alien.y -= alien.vy * dt;
                break;
            case SnakeWave: {
                alien.movementTimer += dt;

                const int row = i / NUM_ALIENS_X;
                const int col = i % NUM_ALIENS_X;

                const float basePhase = alien.movementTimer * alien.frequency;
                const float rowPhase = row * 0.45f;
                const float colPhase = col * 0.25f;

                const float primaryWave = std::sin(basePhase + rowPhase);
                const float secondaryWave = std::sin(basePhase * 0.65f + colPhase);

                alien.x = alien.baseX +
                          alien.amplitude * (0.75f * primaryWave + 0.35f * secondaryWave);

                const float verticalBobVelocity =
                        std::cos(basePhase + rowPhase) * alien.frequency * 0.12f;
                alien.y -= alien.vy * dt;
                alien.y += verticalBobVelocity * dt;

                alien.x += 0.05f * std::sin(basePhase * 1.8f + colPhase + rowPhase);
                break;
            }
            case JustGoDown:
                alien.y -= alien.vy * dt;
                break;
            case Circle:
                break;
            case LeftRight:
                alien.x = std::clamp(alien.x, -0.85f, 0.85f);
                alien.x += alien.movementSpeed * alienDirection_ * dt;
                if (alien.x > 0.85f || alien.x < -0.85f) {
                    hitEdge = true;
                } else if (alien.unitType == UnitType::Relic) {
                    // randomly spike speed and occasionally flip direction to keep boss unpredictable
                    const float roll = Util::getRandomFloat(0.0f, 1.0f);
                    const float eventChance = 0.35f * dt;  // ~35% chance per second
                    if (roll < eventChance) {
                        const float speedBoost = Util::getRandomFloat(1.15f, 1.6f);
                        alien.movementSpeed = std::clamp(alien.movementSpeed * speedBoost, 0.25f, 1.2f);
                        if (Util::getRandomFloat(0.0f, 1.0f) < 0.55f) {
                            alienDirection_ *= -1.0f;
                        }
                    }

                }
                break;
        }
    }

    if (hitEdge) {
        alienDirection_ *= -1.0f;
        for (auto &alien: aliens_) {
            if (alien.active && alien.unitType == UnitType::Standard) {
                alien.y -= 0.04f;
            }
        }
    }
}

bool AlienManager::hasActiveAliens() const {
    return std::any_of(aliens_.begin(), aliens_.end(), [](const Alien &alien) { return alien.active; });
}

bool AlienManager::hasAlienBelow(float threshold) const {
    return std::any_of(aliens_.begin(), aliens_.end(), [threshold](const Alien &alien) {
        return alien.active && alien.y < threshold;
    });
}

std::span<Alien> AlienManager::aliens() {
    return aliens_;
}

std::span<const Alien> AlienManager::aliens() const {
    return aliens_;
}

std::span<MainPushConstants> AlienManager::pushConstants() {
    return alienPC_;
}

std::span<const MainPushConstants> AlienManager::pushConstants() const {
    return alienPC_;
}

void AlienManager::flashAlien(uint32_t index) {
    if (index < alienPC_.size()) {
        alienPC_[index].flashAmount = 1.0f;
    }
}

const Alien* AlienManager::randomActiveAlien() const {
    for (int attempt = 0; attempt < MAX_ALIENS; ++attempt) {
        uint32_t idx = Util::getRandomUint(0, MAX_ALIENS - 1);
        const auto &alien = aliens_[idx];
        if (alien.active) {
            return &alien;
        }
    }

    for (const auto &alien: aliens_) {
        if (alien.active) {
            return &alien;
        }
    }
    return nullptr;
}

void AlienManager::buildBossAlien() {
    Resistances enemyRes{};
    enemyRes.byType[(int) DamageType::Kinetic] = 0.50f;
    enemyRes.byType[(int) DamageType::Fire] = 0.10f;
    enemyRes.byType[(int) DamageType::Lightning] = 0.05f;
    enemyRes.byType[(int) DamageType::Cold] = 0.00f;
    enemyRes.byType[(int) DamageType::Poison] = 0.00f;
    enemyRes.byType[(int) DamageType::Radiation] = 0.15f;
    enemyRes.byType[(int) DamageType::Plasma] = 0.05f;
    enemyRes.byType[(int) DamageType::DarkMatter] = -0.10f; // vulnerable
    enemyRes.byType[(int) DamageType::Cosmic] = 0.20f;

    auto &bossAlien = aliens_[0];
    bossAlien.unitType = UnitType::Relic;
    bossAlien.x = 0.0f;
    bossAlien.y = 0.5f;
    bossAlien.baseX = 0.0f;
    bossAlien.movementTimer = 0.0f;
    bossAlien.amplitude = 0.5f;
    bossAlien.frequency = 0.1f;
    bossAlien.baseFrequency = 0.1f;
    bossAlien.movementType = AlienMovementType::LeftRight;
    bossAlien.active = true;
    bossAlien.health.hull = 1000.0f;
    bossAlien.health.dead = false;
    bossAlien.resistances = enemyRes;
    bossAlien.ailments = {};


    bossAlien.widthHeight = Util::getQuadWidthHeight(quadVerts, 6, {1.0, 1.0});
    alienPC_[0].texturePos = 1;






}

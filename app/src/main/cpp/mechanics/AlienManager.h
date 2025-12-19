#pragma once

#include <array>
#include <optional>
#include <span>

#include "GameObjectData.h"
#include "PowerUpManager.h"
#include "ecs/components/CombatComponents.h"

struct Alien {
    float x{}, y{}, vx{0.1f}, vy{0.02f};
    float movementTimer = 0.0f;   // Used for sine phase
    float amplitude = 0.5f;      // Sine wave width (tune for look)
    float frequency = 0.1f;       // Sine wave speed
    float baseFrequency = 0.1f;
    float baseX = 0.0f;
    float movementSpeed = 0.3f;
    UnitType unitType {UnitType::Standard};
    AlienMovementType movementType{AlienMovementType::LeftRight};
    bool active{};
    std::array<float, 2> widthHeight{};

    Health health;
    Armor armor;
    Resistances resistances;
    Ailments ailments;
};

class AlienManager {
public:
    AlienManager(std::shared_ptr<PowerUpManager> powerUpManager);

    void initAliens();
    void update(float dt);

    bool hasActiveAliens() const;
    bool hasAlienBelow(float threshold) const;

    std::span<Alien> aliens();
    std::span<const Alien> aliens() const;

    std::span<MainPushConstants> pushConstants();
    std::span<const MainPushConstants> pushConstants() const;

    void flashAlien(uint32_t index);
    void resetMovementState();

    const Alien* randomActiveAlien() const;

private:
    std::shared_ptr<PowerUpManager> powerUpManager_;

    std::array<Alien, MAX_ALIENS> aliens_{};
    std::array<MainPushConstants, MAX_ALIENS> alienPC_{};

    float alienDirection_ = 1.0f;
    uint32_t wave_ = 0;
    uint32_t level_ = 0;

    void buildBossAlien();
};

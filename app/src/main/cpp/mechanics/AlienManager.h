#pragma once

#include <array>
#include <optional>
#include <span>

#include "GameObjectData.h"
#include "PowerUpManager.h"
#include "ecs/components/CombatComponents.h"

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

    std::optional<glm::vec2> randomActiveAlienPos() const;

private:
    std::shared_ptr<PowerUpManager> powerUpManager_;

    std::array<Alien, MAX_ALIENS> aliens_{};
    std::array<MainPushConstants, MAX_ALIENS> alienPC_{};

    float alienMoveSpeed_ = 0.3f;
    float alienDirection_ = 1.0f;
    uint32_t wave_ = 0;
    uint32_t level_ = 0;
};

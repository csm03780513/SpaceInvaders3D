#pragma once

#include <array>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>

#include <glm/vec2.hpp>

#include "GameConstants.h"
#include "GameObjectData.h"
#include "ecs/components/CombatComponents.h"
#include "ecs/systems/AlienMovementSystem.h"
#include "ecs/worlds/AlienWorld.h"

class EventBus;
class ParticleSystem;
class IPlatformServices;

class AlienManager {
public:
    AlienManager() = default;

    [[nodiscard]] std::span<Alien> aliens();
    [[nodiscard]] std::span<const Alien> aliens() const;

    [[nodiscard]] std::span<MainPushConstants> pushConstants();
    [[nodiscard]] std::span<const MainPushConstants> pushConstants() const;

    void resetMovement();
    void loadConfig(IPlatformServices &platformServices);
    void initAliens();
    void update(float deltaTime);

    void processCollisions(std::span<Bullet> bullets,
                           Ship &ship,
                           MainPushConstants &shipPushConstants,
                           bool shieldActive,
                           ParticleSystem &particleSystem,
                           EventBus &eventBus);

    [[nodiscard]] std::optional<glm::vec2> updateAndMaybeFire(bool isPlaying, float deltaTime);

    [[nodiscard]] bool hasActiveAliens() const;
    [[nodiscard]] bool hasAlienBelow(float threshold) const;

private:
    static bool isShipBulletHittingAlien(const Alien &alien, const Bullet &bullet);
    static bool isAlienBulletHittingShip(const Ship &ship, const Bullet &bullet);

    struct WaveRule {
        enum class Type { Fixed, RandomWeighted };
        Type type = Type::Fixed;
        AlienMovementType fixedMovement = AlienMovementType::LeftRight;
        std::unordered_map<AlienMovementType, uint32_t> weights{};

        // Optional setup knobs applied when a movement type is chosen.
        float frequencyMul = 1.0f;
        float frequencyAddPerLevel = 0.0f;
        float vyAdd = 0.0f;
        std::optional<float> setX{};
    };

    static std::optional<AlienMovementType> parseMovementType(const std::string &name);
    static AlienMovementType pickWeighted(const std::unordered_map<AlienMovementType, uint32_t> &weights);
    static void applyRuleSetup(const WaveRule &rule, Alien &alien, uint32_t level);

    ecs::AlienWorld world_{};
    ecs::AlienMovementSystem movementSystem_{};

    uint32_t wave_ = 0;
    uint32_t numOfAliens_ = 100;
    uint32_t level_ = 0;

    float fireTimer_ = 0.0f;
    float fireInterval_ = 1.0f;

    std::vector<WaveRule> waveRules_{};
};

#pragma once

#include <array>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/vec2.hpp>

#include "GameConstants.h"
#include "ecs/worlds/GameWorld.h"

class EventBus;
class ParticleSystem;
class PowerUpManager;
class IPlatformServices;

class GameWorldManager {
public:
    GameWorldManager() = default;

    [[nodiscard]] ecs::GameWorld &world();
    [[nodiscard]] const ecs::GameWorld &world() const;

    [[nodiscard]] ecs::EntityId shipEntity() const;

    void loadAlienConfig(IPlatformServices &platformServices);

    void initShip();
    void initAliens();

    void setBulletWidthHeight(const std::array<float, 2> &widthHeight);
    void setBulletSpeeds(float shipBulletSpeed, float alienBulletSpeed);

    void updateAliens(float deltaTime);
    void updateBullets(float deltaTime);
    void decayFlash(float deltaTime);

    void updateAndMaybeFire(bool isPlaying, float deltaTime);

    void processCollisions(bool shieldActive,
                           ParticleSystem &particleSystem,
                           EventBus &eventBus);

    // Spawning bullets is part of the world (returns entity id).
    std::optional<ecs::EntityId> spawnBullet(BulletType type, const glm::vec2 &pos, const DamagePayload &payload);

    [[nodiscard]] bool hasActiveAliens() const;
    [[nodiscard]] bool hasAlienBelow(float threshold) const;

    void destroyEntity(ecs::EntityId entity);

private:
    struct WaveRule {
        enum class Type { Fixed, RandomWeighted };
        Type type = Type::Fixed;
        AlienMovementType fixedMovement = AlienMovementType::LeftRight;
        std::unordered_map<AlienMovementType, uint32_t> weights{};

        float frequencyMul = 1.0f;
        float frequencyAddPerLevel = 0.0f;
        float vyAdd = 0.0f;
        std::optional<float> setX{};
    };

    static std::optional<AlienMovementType> parseMovementType(const std::string &name);
    static AlienMovementType pickWeighted(const std::unordered_map<AlienMovementType, uint32_t> &weights);
    static void applyRuleSetup(const WaveRule &rule, Alien &alien, uint32_t level);

    static bool isShipBulletHittingAlien(const Alien &alien, const Bullet &bullet);
    static bool isAlienBulletHittingShip(const Ship &ship, const Bullet &bullet);

    void resetWorldForNewWave();
    void updateAlienMovement(float deltaTime);
    void updateBulletMovement(float deltaTime);

    ecs::GameWorld world_{};

    std::vector<WaveRule> waveRules_{};
    uint32_t wave_ = 0;
    uint32_t level_ = 0;

    std::array<ecs::EntityId, MAX_ALIENS> alienEntities_{};
    std::array<float, 2> bulletWidthHeight_{};
    float shipBulletSpeed_ = 2.0f;
    float alienBulletSpeed_ = 0.5f;

    float alienMoveSpeed_ = 0.3f;
    float alienDirection_ = 1.0f;

    float fireTimer_ = 0.0f;
    float fireInterval_ = 1.0f;
};

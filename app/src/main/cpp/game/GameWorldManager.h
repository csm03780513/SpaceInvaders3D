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
#include "ecs/worlds/PrefabLibrary.h"

class EventBus;
class ParticleSystem;
class PowerUpManager;
class IPlatformServices;

class GameWorldManager {
public:
    GameWorldManager();

    [[nodiscard]] ecs::GameWorld &world();
    [[nodiscard]] const ecs::GameWorld &world() const;

    [[nodiscard]] ecs::EntityId shipEntity() const;

    void loadPrefabs(IPlatformServices &platformServices);
    void loadAlienConfig(IPlatformServices &platformServices);

    void initShip();
    void initAliens();

    void setBulletWidthHeight(const std::array<float, 2> &widthHeight);

    void updateAliens(float deltaTime);
    void updateBullets(float deltaTime);
    void decayFlash(float deltaTime);

    void updateAndMaybeFire(bool isPlaying, float deltaTime);

    void processCollisions(bool shieldActive,
                           ParticleSystem &particleSystem,
                           EventBus &eventBus);

    // Spawning bullets is part of the world (returns entity id).
    std::optional<ecs::EntityId> spawnBullet(const std::string &prefabName, const glm::vec2 &pos);

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

    struct WaveDefinition {
        std::string name{"wave"};
        std::string prefabName{"grunt"};
        std::string bulletPrefab{"alien_primary"};
        uint32_t rows = NUM_ALIENS_Y;
        uint32_t cols = NUM_ALIENS_X;
        glm::vec2 start{-0.7f, 0.8f};
        glm::vec2 spacing{0.2f, 0.15f};
        WaveRule rule{};
        std::vector<std::string> modifiers{};
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
    ecs::PrefabLibrary prefabs_{};

    std::vector<WaveDefinition> waveRules_{};
    uint32_t wave_ = 0;
    uint32_t level_ = 0;

    std::array<float, 2> bulletWidthHeight_{};

    float alienMoveSpeed_ = 0.3f;
    float alienDirection_ = 1.0f;

    float fireTimer_ = 0.0f;
    float fireInterval_ = 1.0f;
    std::string shipBulletPrefab_{"ship_primary"};
    std::string shipDualBulletPrefab_{"ship_dual"};
    std::string activeAlienBulletPrefab_{"alien_primary"};
}; 

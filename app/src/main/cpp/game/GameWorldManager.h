#pragma once

#include <array>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/vec2.hpp>

#include "GameConstants.h"
#include "ecs/components/GameplayComponents.h"
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

    [[nodiscard]] std::optional<ecs::EntityId> shipEntity() const;

    void loadPrefabs(IPlatformServices &platformServices);
    void loadAlienConfig(IPlatformServices &platformServices);

    void initShip();
    void initAliens();

    void setBulletWidthHeight(const std::array<float, 2> &widthHeight);

    void decayFlash(float deltaTime);

    void processCollisions(bool shieldActive,
                           ParticleSystem &particleSystem,
                           EventBus &eventBus);

    // Spawning bullets is part of the world (returns entity id).
    std::optional<ecs::EntityId> spawnBullet(const std::string &prefabName, const glm::vec2 &pos);

    [[nodiscard]] bool hasActiveAliens() const;
    [[nodiscard]] bool hasAlienBelow(float threshold) const;

    void destroyEntity(ecs::EntityId entity);

    [[nodiscard]] std::optional<ecs::EntityId> settingsEntity() const;
    [[nodiscard]] ecs::PrefabLibrary &prefabs();
    [[nodiscard]] const ecs::PrefabLibrary &prefabs() const;

private:
    static bool isShipBulletHittingAlien(const Alien &alien, const Bullet &bullet);
    static bool isAlienBulletHittingShip(const Ship &ship, const Bullet &bullet);

    void resetWorldForNewWave();
    void syncWaveSettings();

    ecs::GameWorld world_{};
    ecs::PrefabLibrary prefabs_{};

    WaveSettings cachedWaveSettings_{};
    std::optional<ecs::EntityId> settingsEntity_{};

    std::array<float, 2> bulletWidthHeight_{};
};

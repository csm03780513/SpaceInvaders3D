#include "GameWorldManager.h"

#include <algorithm>

#include <glm/vec3.hpp>

#include "Collision.h"
#include "Util.h"
#include "ecs/components/GameplayComponents.h"
#include "ecs/systems/AlienSpawnSystem.h"
#include "events/EventBus.h"
#include "json/TinyJson.h"
#include "mechanics/Damage.h"
#include "platform/PlatformServices.h"

#include "ParticleSystem.h"

GameWorldManager::GameWorldManager() {
    prefabs_.loadDefaults();
    bulletWidthHeight_ = {0.04f, 0.05f};
}

ecs::GameWorld &GameWorldManager::world() {
    return world_;
}

const ecs::GameWorld &GameWorldManager::world() const {
    return world_;
}

std::optional<ecs::EntityId> GameWorldManager::shipEntity() const {
    std::optional<ecs::EntityId> ship{};
    const auto &ships = world_.pool<Ship>();
    world_.registry.forEachAlive([&](ecs::EntityId e) {
        if (ship.has_value()) return;
        if (ships.has(e)) ship = e;
    });
    return ship;
}

void GameWorldManager::loadPrefabs(IPlatformServices &platformServices) {
    prefabs_.loadFromJson(platformServices);
}

void GameWorldManager::setBulletWidthHeight(const std::array<float, 2> &widthHeight) {
    bulletWidthHeight_ = widthHeight;
}

void GameWorldManager::loadAlienConfig(IPlatformServices &platformServices) {
    cachedWaveSettings_.waves.clear();

    std::vector<uint8_t> bytes;
    try {
        bytes = platformServices.loadAsset("config/alien_waves.json");
    } catch (...) {
        return;
    }
    if (bytes.empty()) {
        return;
    }

    std::string text(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    try {
        const TinyJson::Value root = TinyJson::parse(text);
        if (!root.isObject()) return;
        const auto &obj = root.asObject();
        const TinyJson::Value *wavesVal = TinyJson::get(obj, "waves");
        if (!wavesVal || !wavesVal->isArray()) return;

        for (const auto &waveValue : wavesVal->asArray()) {
            if (!waveValue.isObject()) continue;
            const auto &waveObj = waveValue.asObject();

            WaveDefinition def{};
            def.name = TinyJson::getString(waveObj, "name").value_or(def.name);
            def.prefabName = TinyJson::getString(waveObj, "prefab").value_or(def.prefabName);
            def.bulletPrefab = TinyJson::getString(waveObj, "bulletPrefab").value_or(def.bulletPrefab);
            def.rows = static_cast<uint32_t>(TinyJson::getNumber(waveObj, "rows").value_or(def.rows));
            def.cols = static_cast<uint32_t>(TinyJson::getNumber(waveObj, "cols").value_or(def.cols));

            if (const TinyJson::Value *start = TinyJson::get(waveObj, "start")) {
                if (start->isObject()) {
                    def.start.x = static_cast<float>(TinyJson::getNumber(start->asObject(), "x").value_or(def.start.x));
                    def.start.y = static_cast<float>(TinyJson::getNumber(start->asObject(), "y").value_or(def.start.y));
                }
            }
            if (const TinyJson::Value *spacing = TinyJson::get(waveObj, "spacing")) {
                if (spacing->isObject()) {
                    def.spacing.x = static_cast<float>(TinyJson::getNumber(spacing->asObject(), "x").value_or(def.spacing.x));
                    def.spacing.y = static_cast<float>(TinyJson::getNumber(spacing->asObject(), "y").value_or(def.spacing.y));
                }
            }

            WaveRule rule{};
            const TinyJson::Value *movementVal = TinyJson::get(waveObj, "movement");
            std::string typeStr = movementVal && movementVal->isObject()
                                  ? TinyJson::getString(movementVal->asObject(), "type").value_or("fixed")
                                  : TinyJson::getString(waveObj, "type").value_or("fixed");
            if (typeStr == "randomWeighted") {
                rule.type = WaveRule::Type::RandomWeighted;
                if (movementVal && movementVal->isObject()) {
                    if (const TinyJson::Value *weightsVal = TinyJson::get(movementVal->asObject(), "weights")) {
                        if (weightsVal->isObject()) {
                            for (const auto &[k, v] : weightsVal->asObject()) {
                                if (!v.isNumber()) continue;
                                auto mt = ecs::AlienSpawnSystem::parseMovementType(k);
                                if (!mt.has_value()) continue;
                                const auto w = static_cast<uint32_t>(std::max(0.0, v.asNumber()));
                                rule.weights[*mt] = w;
                            }
                        }
                    }
                }
            } else {
                rule.type = WaveRule::Type::Fixed;
                if (movementVal && movementVal->isObject()) {
                    const auto movementStr = TinyJson::getString(movementVal->asObject(), "type");
                    if (movementStr.has_value()) {
                        auto mt = ecs::AlienSpawnSystem::parseMovementType(*movementStr);
                        if (mt.has_value()) rule.fixedMovement = *mt;
                    }
                    if (auto v = TinyJson::getNumber(movementVal->asObject(), "frequencyMul")) rule.frequencyMul = static_cast<float>(*v);
                    if (auto v = TinyJson::getNumber(movementVal->asObject(), "frequencyAddPerLevel")) rule.frequencyAddPerLevel = static_cast<float>(*v);
                    if (auto v = TinyJson::getNumber(movementVal->asObject(), "vyAdd")) rule.vyAdd = static_cast<float>(*v);
                    if (auto v = TinyJson::getNumber(movementVal->asObject(), "setX")) rule.setX = static_cast<float>(*v);
                }
            }
            def.rule = rule;

            if (const TinyJson::Value *mods = TinyJson::get(waveObj, "modifiers")) {
                if (mods->isArray()) {
                    for (const auto &m : mods->asArray()) {
                        if (m.isString()) def.modifiers.push_back(m.asString());
                    }
                }
            }

            cachedWaveSettings_.waves.push_back(std::move(def));
        }
    } catch (...) {
        cachedWaveSettings_.waves.clear();
    }

    syncWaveSettings();
}

void GameWorldManager::syncWaveSettings() {
    if (!settingsEntity_.has_value() || !world_.registry.alive(*settingsEntity_)) {
        auto entity = world_.registry.create();
        if (!entity.has_value()) {
            return;
        }
        settingsEntity_ = entity;
    }

    auto &wavePool = world_.pool<WaveSettings>();
    wavePool.add(*settingsEntity_, cachedWaveSettings_);

    auto &movement = world_.pool<DirectionalMovement>();
    if (!movement.has(*settingsEntity_)) {
        movement.add(*settingsEntity_, DirectionalMovement{});
    }

    auto &cooldown = world_.pool<FireCooldown>();
    if (!cooldown.has(*settingsEntity_)) {
        cooldown.add(*settingsEntity_, FireCooldown{});
    }
}

void GameWorldManager::initShip() {
    auto shipId = shipEntity();
    if (!shipId.has_value() || !world_.registry.alive(*shipId)) {
        const auto e = world_.registry.create();
        if (!e.has_value()) {
            return;
        }
        shipId = e;
    }

    auto &ships = world_.pool<Ship>();
    auto &render = world_.pool<MainPushConstants>();

    const auto &prefab = prefabs_.ship("player");
    ships.add(*shipId, prefab.ship);
    render.add(*shipId, prefab.render);
}

void GameWorldManager::resetWorldForNewWave() {
    if (settingsEntity_.has_value()) {
        auto &wavePool = world_.pool<WaveSettings>();
        if (wavePool.has(*settingsEntity_)) {
            cachedWaveSettings_ = wavePool.get(*settingsEntity_);
        }
    }

    world_.reset();
    settingsEntity_.reset();

    syncWaveSettings();
    initShip();
}

void GameWorldManager::initAliens() {
    resetWorldForNewWave();

    if (!settingsEntity_.has_value()) {
        return;
    }

    auto &wavePool = world_.pool<WaveSettings>();
    WaveSettings &settings = wavePool.add(*settingsEntity_, cachedWaveSettings_);
    settings.level++;
    settings.needsSpawn = true;
    cachedWaveSettings_ = settings;

    auto &movement = world_.pool<DirectionalMovement>();
    movement.add(*settingsEntity_, DirectionalMovement{});

    auto &cooldown = world_.pool<FireCooldown>();
    cooldown.add(*settingsEntity_, FireCooldown{});
}

void GameWorldManager::decayFlash(float deltaTime) {
    auto &render = world_.pool<MainPushConstants>();
    world_.registry.forEachAlive([&](ecs::EntityId e) {
        if (!render.has(e)) return;
        auto &pc = render.get(e);
        pc.flashAmount -= deltaTime * 5.0f;
        if (pc.flashAmount < 0.0f) pc.flashAmount = 0.0f;
    });
}

std::optional<ecs::EntityId> GameWorldManager::spawnBullet(const std::string &prefabName, const glm::vec2 &pos) {
    const auto e = world_.registry.create();
    if (!e.has_value()) {
        return std::nullopt;
    }

    auto &bullets = world_.pool<Bullet>();
    auto &render = world_.pool<MainPushConstants>();

    const auto &prefab = prefabs_.bullet(prefabName);
    Bullet bullet{};
    bullet.active = true;
    bullet.bulletType = prefab.type;
    bullet.x = pos.x;
    bullet.y = pos.y;
    bullet.payload = prefab.payload;
    bullet.widthHeight = prefab.size[0] > 0.0f && prefab.size[1] > 0.0f ? prefab.size : bulletWidthHeight_;
    bullet.speed = prefab.speed;

    bullets.add(*e, bullet);
    render.add(*e, MainPushConstants{});
    return *e;
}

bool GameWorldManager::isShipBulletHittingAlien(const Alien &alien, const Bullet &bullet) {
    auto alienAABB = Collision::getAABB(alien.x, alien.y, alien.widthHeight[0], alien.widthHeight[1]);
    auto bulletAABB = Collision::getAABB(bullet.x, -bullet.y, bullet.widthHeight[0], bullet.widthHeight[1]);
    return Collision::isColliding(alienAABB, bulletAABB);
}

bool GameWorldManager::isAlienBulletHittingShip(const Ship &ship, const Bullet &bullet) {
    auto shipAABB = Collision::getAABB(ship.x, ship.y, ship.widthHeight[0], ship.widthHeight[1]);
    auto bulletAABB = Collision::getAABB(bullet.x, bullet.y, bullet.widthHeight[0], bullet.widthHeight[1]);
    return Collision::isColliding(shipAABB, bulletAABB);
}

void GameWorldManager::processCollisions(bool shieldActive,
                                        ParticleSystem &particleSystem,
                                        EventBus &eventBus) {
    auto shipId = shipEntity();
    if (!shipId.has_value() || !world_.registry.alive(*shipId)) return;

    auto &ships = world_.pool<Ship>();
    auto &aliens = world_.pool<Alien>();
    auto &bullets = world_.pool<Bullet>();
    auto &render = world_.pool<MainPushConstants>();

    Ship *ship = ships.tryGet(*shipId);
    if (!ship) return;

    std::vector<ecs::EntityId> bulletsToDestroy;
    bulletsToDestroy.reserve(8);

    std::vector<std::pair<ecs::EntityId, Alien *>> activeAliens;
    world_.registry.forEachAlive([&](ecs::EntityId e) {
        Alien *alien = aliens.tryGet(e);
        if (alien && alien->active) {
            activeAliens.emplace_back(e, alien);
        }
    });

    world_.registry.forEachAlive([&](ecs::EntityId be) {
        Bullet *bullet = bullets.tryGet(be);
        if (!bullet || !bullet->active) return;

        if (bullet->bulletType == BulletType::Ship) {
            for (auto &[ae, alien] : activeAliens) {
                if (!isShipBulletHittingAlien(*alien, *bullet)) continue;

                bullet->active = false;
                bulletsToDestroy.push_back(be);

                eventBus.publish(HitEvent{
                        .attacker = *shipId,
                        .target = ae,
                        .payload = bullet->payload,
                        .hitWorldPos = glm::vec2(alien->x, alien->y),
                });

                if (render.has(ae)) {
                    render.get(ae).flashAmount = 1.0f;
                }
                particleSystem.spawn(glm::vec3(alien->x, -alien->y, 1.0f), 5);
                break;
            }
            return;
        }

        if (bullet->bulletType == BulletType::Alien && !shieldActive) {
            if (isAlienBulletHittingShip(*ship, *bullet)) {
                bullet->active = false;
                bulletsToDestroy.push_back(be);
                eventBus.publish(HitEvent{
                        .attacker = 0,
                        .target = *shipId,
                        .payload = bullet->payload,
                        .hitWorldPos = glm::vec2(ship->x, ship->y),
                });

                if (render.has(*shipId)) {
                    render.get(*shipId).flashAmount = 1.0f;
                }
                particleSystem.spawn(glm::vec3(bullet->x, bullet->y, 0.0f), 10);
            }
        }
    });

    for (auto be : bulletsToDestroy) {
        destroyEntity(be);
    }
}

bool GameWorldManager::hasActiveAliens() const {
    bool hasAliens = false;
    const auto &aliens = world_.pool<Alien>();
    world_.registry.forEachAlive([&](ecs::EntityId e) {
        const Alien *a = aliens.tryGet(e);
        if (a && a->active) hasAliens = true;
    });
    return hasAliens;
}

bool GameWorldManager::hasAlienBelow(float threshold) const {
    bool below = false;
    const auto &aliens = world_.pool<Alien>();
    world_.registry.forEachAlive([&](ecs::EntityId e) {
        const Alien *a = aliens.tryGet(e);
        if (a && a->active && a->y < threshold) below = true;
    });
    return below;
}

void GameWorldManager::destroyEntity(ecs::EntityId entity) {
    if (!world_.registry.alive(entity)) return;

    world_.pool<Ship>().remove(entity);
    world_.pool<Alien>().remove(entity);
    world_.pool<Bullet>().remove(entity);
    world_.pool<MainPushConstants>().remove(entity);
    world_.registry.destroy(entity);
}

std::optional<ecs::EntityId> GameWorldManager::settingsEntity() const {
    return settingsEntity_;
}

ecs::PrefabLibrary &GameWorldManager::prefabs() {
    return prefabs_;
}

const ecs::PrefabLibrary &GameWorldManager::prefabs() const {
    return prefabs_;
}


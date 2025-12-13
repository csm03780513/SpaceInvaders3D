#include "GameWorldManager.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include <glm/vec3.hpp>

#include "Collision.h"
#include "Util.h"
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

ecs::EntityId GameWorldManager::shipEntity() const {
    return world_.shipEntity;
}

void GameWorldManager::loadPrefabs(IPlatformServices &platformServices) {
    prefabs_.loadFromJson(platformServices);
}

void GameWorldManager::setBulletWidthHeight(const std::array<float, 2> &widthHeight) {
    bulletWidthHeight_ = widthHeight;
}

std::optional<AlienMovementType> GameWorldManager::parseMovementType(const std::string &name) {
    if (name == "SnakeWave") return AlienMovementType::SnakeWave;
    if (name == "JustGoDown") return AlienMovementType::JustGoDown;
    if (name == "TogetherOne") return AlienMovementType::TogetherOne;
    if (name == "SineWave") return AlienMovementType::SineWave;
    if (name == "Circle") return AlienMovementType::Circle;
    if (name == "LeftRight") return AlienMovementType::LeftRight;
    if (name == "MySnakeWave") return AlienMovementType::MySnakeWave;
    return std::nullopt;
}

AlienMovementType GameWorldManager::pickWeighted(const std::unordered_map<AlienMovementType, uint32_t> &weights) {
    uint32_t total = std::accumulate(weights.begin(), weights.end(), 0u,
                                     [](uint32_t acc, const auto &pair) { return acc + pair.second; });
    if (total == 0) {
        return AlienMovementType::JustGoDown;
    }

    const uint32_t roll = Util::getRandomUint(0, total - 1);
    uint32_t acc = 0;
    for (const auto &[type, w] : weights) {
        acc += w;
        if (roll < acc) {
            return type;
        }
    }
    return weights.begin()->first;
}

void GameWorldManager::applyRuleSetup(const WaveRule &rule, Alien &alien, uint32_t level) {
    if (rule.setX.has_value()) {
        alien.x = *rule.setX;
    }

    if (rule.frequencyMul != 1.0f || rule.frequencyAddPerLevel != 0.0f) {
        alien.frequency = alien.baseFrequency * rule.frequencyMul + (static_cast<float>(level) * rule.frequencyAddPerLevel);
    }

    alien.vy += rule.vyAdd;
}

void GameWorldManager::loadAlienConfig(IPlatformServices &platformServices) {
    waveRules_.clear();

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
                                auto mt = parseMovementType(k);
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
                        auto mt = parseMovementType(*movementStr);
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

            waveRules_.push_back(std::move(def));
        }
    } catch (...) {
        waveRules_.clear();
    }
}

void GameWorldManager::initShip() {
    if (world_.shipEntity == 0 || !world_.registry.alive(world_.shipEntity)) {
        const auto e = world_.registry.create();
        if (!e.has_value()) {
            return;
        }
        world_.shipEntity = *e;
    }

    const auto &prefab = prefabs_.ship("player");
    world_.ships.add(world_.shipEntity, prefab.ship);
    world_.render.add(world_.shipEntity, prefab.render);
}

void GameWorldManager::resetWorldForNewWave() {
    world_.registry.reset();
    world_.ships.reset();
    world_.aliens.reset();
    world_.bullets.reset();
    world_.render.reset();

    initShip();
}

static void applyModifiers(const ecs::PrefabLibrary &library, const std::vector<std::string> &mods, Alien &alien) {
    for (const auto &name : mods) {
        const auto *mod = library.modifier(name);
        if (!mod) continue;
        for (size_t i = 0; i < std::size(alien.resistances.byType); ++i) {
            alien.resistances.byType[i] += mod->resistanceDelta.byType[i];
        }
        alien.armor.flatReduction += mod->armorDelta.flatReduction;
        // Apply any ambient ailments the modifier wants the alien to start with.
        alien.ailments = mod->ailments;
    }
}

void GameWorldManager::initAliens() {
    if (!world_.registry.alive(world_.shipEntity) || !world_.ships.has(world_.shipEntity)) {
        initShip();
    }

    resetWorldForNewWave();

    level_++;
    if (waveRules_.empty()) {
        WaveDefinition fallback{};
        waveRules_.push_back(fallback);
    }

    const WaveDefinition &wave = waveRules_[wave_ % waveRules_.size()];
    activeAlienBulletPrefab_ = wave.bulletPrefab;

    for (uint32_t r = 0; r < wave.rows; ++r) {
        for (uint32_t c = 0; c < wave.cols; ++c) {
            const auto e = world_.registry.create();
            if (!e.has_value()) continue;

            Alien alien = prefabs_.alien(wave.prefabName).alien;
            alien.x = wave.start.x + static_cast<float>(c) * wave.spacing.x;
            alien.y = wave.start.y - static_cast<float>(r) * wave.spacing.y;
            alien.baseX = alien.x;
            alien.active = true;
            alien.spawnRow = static_cast<uint16_t>(r);
            alien.spawnCol = static_cast<uint16_t>(c);

            WaveRule rule = wave.rule;
            if (rule.type == WaveRule::Type::RandomWeighted) {
                alien.movementType = pickWeighted(rule.weights);
            } else {
                alien.movementType = rule.fixedMovement;
            }
            applyRuleSetup(rule, alien, level_);
            applyModifiers(prefabs_, wave.modifiers, alien);

            const auto &prefab = prefabs_.alien(wave.prefabName);
            auto &pc = world_.render.add(*e, prefab.render);
            pc.texturePos = prefab.render.texturePos;

            world_.aliens.add(*e, alien);
        }
    }

    wave_++;
    fireTimer_ = 0.0f;
    alienMoveSpeed_ = 0.3f;
    alienDirection_ = 1.0f;
}

void GameWorldManager::decayFlash(float deltaTime) {
    world_.registry.forEachAlive([&](ecs::EntityId e) {
        if (!world_.render.has(e)) return;
        auto &pc = world_.render.get(e);
        pc.flashAmount -= deltaTime * 5.0f;
        if (pc.flashAmount < 0.0f) pc.flashAmount = 0.0f;
    });
}

void GameWorldManager::updateAliens(float deltaTime) {
    updateAlienMovement(deltaTime);
}

void GameWorldManager::updateBullets(float deltaTime) {
    updateBulletMovement(deltaTime);
}

void GameWorldManager::updateAlienMovement(float deltaTime) {
    bool hitEdge = false;

    world_.registry.forEachAlive([&](ecs::EntityId e) {
        Alien *alien = world_.aliens.tryGet(e);
        if (!alien || !alien->active) return;

        if (world_.render.has(e)) {
            auto &pc = world_.render.get(e);
            pc.flashAmount -= deltaTime * 5.0f;
            if (pc.flashAmount < 0.0f) pc.flashAmount = 0.0f;
        }

        switch (alien->movementType) {
            case TogetherOne:
                alien->y -= alien->vy * deltaTime;
                break;
            case SineWave:
                alien->movementTimer += deltaTime;
                alien->x = alien->baseX + alien->amplitude * std::sin(alien->movementTimer * alien->frequency);
                alien->y -= alien->vy * deltaTime;
                break;
            case MySnakeWave:
                alien->movementTimer += deltaTime;
                alien->x = std::sin((alien->movementTimer + alien->baseX) * alien->frequency);
                alien->y -= alien->vy * deltaTime;
                break;
            case SnakeWave: {
                alien->movementTimer += deltaTime;

                const int row = static_cast<int>(alien->spawnRow);
                const int col = static_cast<int>(alien->spawnCol);

                const float basePhase = alien->movementTimer * alien->frequency;
                const float rowPhase = row * 0.45f;
                const float colPhase = col * 0.25f;

                const float primaryWave = std::sin(basePhase + rowPhase);
                const float secondaryWave = std::sin(basePhase * 0.65f + colPhase);

                alien->x = alien->baseX + alien->amplitude * (0.75f * primaryWave + 0.35f * secondaryWave);

                const float verticalBobVelocity = std::cos(basePhase + rowPhase) * alien->frequency * 0.12f;
                alien->y -= alien->vy * deltaTime;
                alien->y += verticalBobVelocity * deltaTime;

                alien->x += 0.05f * std::sin(basePhase * 1.8f + colPhase + rowPhase);
                break;
            }
            case JustGoDown:
                alien->y -= alien->vy * deltaTime;
                break;
            case Circle:
                break;
            case LeftRight:
                alien->x += alienMoveSpeed_ * alienDirection_ * deltaTime;
                if (alien->x > 0.85f) alien->x = 0.85f;
                if (alien->x < -0.85f) alien->x = -0.85f;
                if (alien->x > 0.84f || alien->x < -0.84f) {
                    hitEdge = true;
                }
                break;
        }
    });

    if (hitEdge) {
        alienDirection_ *= -1.0f;
        world_.registry.forEachAlive([&](ecs::EntityId e) {
            Alien *alien = world_.aliens.tryGet(e);
            if (!alien || !alien->active) return;
            alien->y -= 0.04f;
        });
    }
}

void GameWorldManager::updateBulletMovement(float deltaTime) {
    std::vector<ecs::EntityId> toDestroy;
    toDestroy.reserve(8);

    world_.registry.forEachAlive([&](ecs::EntityId e) {
        Bullet *b = world_.bullets.tryGet(e);
        if (!b) return;

        if (!b->active) {
            toDestroy.push_back(e);
            return;
        }

        if (b->bulletType == BulletType::Ship) {
            b->y -= b->speed * deltaTime;
        } else if (b->bulletType == BulletType::Alien) {
            b->y += b->speed * deltaTime;
        }

        if ((b->bulletType == BulletType::Ship && b->y < -1.0f) ||
            (b->bulletType == BulletType::Alien && b->y > 1.0f)) {
            b->active = false;
            toDestroy.push_back(e);
        }
    });

    for (auto e : toDestroy) {
        destroyEntity(e);
    }
}

std::optional<ecs::EntityId> GameWorldManager::spawnBullet(const std::string &prefabName, const glm::vec2 &pos) {
    const auto e = world_.registry.create();
    if (!e.has_value()) {
        return std::nullopt;
    }

    const auto &prefab = prefabs_.bullet(prefabName);
    Bullet bullet{};
    bullet.active = true;
    bullet.bulletType = prefab.type;
    bullet.x = pos.x;
    bullet.y = pos.y;
    bullet.payload = prefab.payload;
    bullet.widthHeight = prefab.size[0] > 0.0f && prefab.size[1] > 0.0f ? prefab.size : bulletWidthHeight_;
    bullet.speed = prefab.speed;

    world_.bullets.add(*e, bullet);
    world_.render.add(*e, MainPushConstants{});
    return *e;
}

void GameWorldManager::updateAndMaybeFire(bool isPlaying, float deltaTime) {
    if (!isPlaying) return;
    fireTimer_ += deltaTime;
    if (fireTimer_ <= fireInterval_) return;
    fireTimer_ = 0.0f;

    std::vector<ecs::EntityId> aliveAliens;
    aliveAliens.reserve(8);
    world_.registry.forEachAlive([&](ecs::EntityId e) {
        const Alien *alien = world_.aliens.tryGet(e);
        if (alien && alien->active) aliveAliens.push_back(e);
    });
    if (aliveAliens.empty()) return;

    const uint32_t idx = Util::getRandomUint(0, static_cast<uint32_t>(aliveAliens.size() - 1));
    const Alien &alien = world_.aliens.get(aliveAliens[idx]);
    (void) spawnBullet(activeAlienBulletPrefab_, {alien.x, -alien.y});
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
    Ship *ship = world_.ships.tryGet(world_.shipEntity);
    if (!ship) return;

    std::vector<ecs::EntityId> bulletsToDestroy;
    bulletsToDestroy.reserve(8);

    std::vector<std::pair<ecs::EntityId, Alien *>> activeAliens;
    world_.registry.forEachAlive([&](ecs::EntityId e) {
        Alien *alien = world_.aliens.tryGet(e);
        if (alien && alien->active) {
            activeAliens.emplace_back(e, alien);
        }
    });

    world_.registry.forEachAlive([&](ecs::EntityId be) {
        Bullet *bullet = world_.bullets.tryGet(be);
        if (!bullet || !bullet->active) return;

        if (bullet->bulletType == BulletType::Ship) {
            for (auto &[ae, alien] : activeAliens) {
                if (!isShipBulletHittingAlien(*alien, *bullet)) continue;

                bullet->active = false;
                bulletsToDestroy.push_back(be);

                eventBus.publish(HitEvent{
                        .attacker = world_.shipEntity,
                        .target = ae,
                        .payload = bullet->payload,
                        .hitWorldPos = glm::vec2(alien->x, alien->y),
                });

                if (world_.render.has(ae)) {
                    world_.render.get(ae).flashAmount = 1.0f;
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
                        .target = world_.shipEntity,
                        .payload = bullet->payload,
                        .hitWorldPos = glm::vec2(ship->x, ship->y),
                });

                if (world_.render.has(world_.shipEntity)) {
                    world_.render.get(world_.shipEntity).flashAmount = 1.0f;
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
    world_.registry.forEachAlive([&](ecs::EntityId e) {
        const Alien *a = world_.aliens.tryGet(e);
        if (a && a->active) hasAliens = true;
    });
    return hasAliens;
}

bool GameWorldManager::hasAlienBelow(float threshold) const {
    bool below = false;
    world_.registry.forEachAlive([&](ecs::EntityId e) {
        const Alien *a = world_.aliens.tryGet(e);
        if (a && a->active && a->y < threshold) below = true;
    });
    return below;
}

void GameWorldManager::destroyEntity(ecs::EntityId entity) {
    if (!world_.registry.alive(entity)) return;

    world_.ships.remove(entity);
    world_.aliens.remove(entity);
    world_.bullets.remove(entity);
    world_.render.remove(entity);
    world_.registry.destroy(entity);
}


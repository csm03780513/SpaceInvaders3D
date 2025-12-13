#include "GameWorldManager.h"

#include <algorithm>
#include <cstring>

#include <glm/vec3.hpp>

#include "Collision.h"
#include "Util.h"
#include "events/EventBus.h"
#include "json/TinyJson.h"
#include "mechanics/Damage.h"
#include "platform/PlatformServices.h"

#include "ParticleSystem.h"

ecs::GameWorld &GameWorldManager::world() {
    return world_;
}

const ecs::GameWorld &GameWorldManager::world() const {
    return world_;
}

ecs::EntityId GameWorldManager::shipEntity() const {
    return world_.shipEntity;
}

void GameWorldManager::setBulletWidthHeight(const std::array<float, 2> &widthHeight) {
    bulletWidthHeight_ = widthHeight;
}

void GameWorldManager::setBulletSpeeds(float shipBulletSpeed, float alienBulletSpeed) {
    shipBulletSpeed_ = shipBulletSpeed;
    alienBulletSpeed_ = alienBulletSpeed;
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
    uint32_t total = 0;
    for (const auto &[type, w] : weights) {
        total += w;
    }
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

            WaveRule rule{};

            const auto typeStr = TinyJson::getString(waveObj, "type").value_or("fixed");
            if (typeStr == "randomWeighted") {
                rule.type = WaveRule::Type::RandomWeighted;
                if (const TinyJson::Value *weightsVal = TinyJson::get(waveObj, "weights")) {
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
            } else {
                rule.type = WaveRule::Type::Fixed;
                const auto movementStr = TinyJson::getString(waveObj, "movement");
                if (!movementStr.has_value()) continue;
                auto mt = parseMovementType(*movementStr);
                if (!mt.has_value()) continue;
                rule.fixedMovement = *mt;
            }

            if (auto v = TinyJson::getNumber(waveObj, "frequencyMul")) rule.frequencyMul = static_cast<float>(*v);
            if (auto v = TinyJson::getNumber(waveObj, "frequencyAddPerLevel")) rule.frequencyAddPerLevel = static_cast<float>(*v);
            if (auto v = TinyJson::getNumber(waveObj, "vyAdd")) rule.vyAdd = static_cast<float>(*v);
            if (auto v = TinyJson::getNumber(waveObj, "setX")) rule.setX = static_cast<float>(*v);

            waveRules_.push_back(std::move(rule));
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

    world_.ships.add(world_.shipEntity, Ship{});
    world_.render.add(world_.shipEntity, MainPushConstants{});
}

void GameWorldManager::resetWorldForNewWave() {
    for (auto &e : alienEntities_) {
        e = 0;
    }

    world_.registry.reset();
    world_.ships.reset();
    world_.aliens.reset();
    world_.bullets.reset();
    world_.render.reset();

    initShip();
}

void GameWorldManager::initAliens() {
    if (!world_.registry.alive(world_.shipEntity) || !world_.ships.has(world_.shipEntity)) {
        initShip();
    }

    // Remove any existing aliens/bullets but keep ship (simple approach for now).
    // We reset everything then re-create ship and aliens.
    resetWorldForNewWave();

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

    const float startX = -0.7f;
    const float startY = 0.8f;
    const float dx = 0.2f;
    const float dy = 0.15f;

    level_++;
    if (wave_ >= 5) wave_ = 0;

    for (int y = 0; y < NUM_ALIENS_Y; ++y) {
        for (int x = 0; x < NUM_ALIENS_X; ++x) {
            const int slot = y * NUM_ALIENS_X + x;
            const auto e = world_.registry.create();
            if (!e.has_value()) {
                continue;
            }
            alienEntities_[slot] = *e;

            Alien alien{};
            alien.x = startX + x * dx;
            alien.baseX = alien.x;
            alien.movementTimer = 0.0f;
            alien.y = startY - y * dy;
            alien.active = true;
            alien.health.hull = 100.0f;
            alien.health.dead = false;
            alien.resistances = enemyRes;
            alien.ailments = {};
            alien.widthHeight = Util::getQuadWidthHeight(alienVerts, 6, {1.0f, 1.0f});

            WaveRule rule{};
            if (!waveRules_.empty() && wave_ < waveRules_.size()) {
                rule = waveRules_[wave_];
            } else {
                // fallback
                switch (wave_) {
                    case 0:
                        rule.type = WaveRule::Type::Fixed;
                        rule.fixedMovement = AlienMovementType::LeftRight;
                        break;
                    case 1:
                        rule.type = WaveRule::Type::Fixed;
                        rule.fixedMovement = AlienMovementType::SnakeWave;
                        rule.frequencyMul = 7.0f;
                        rule.frequencyAddPerLevel = 0.05f;
                        rule.vyAdd = 0.01f;
                        break;
                    case 2:
                        rule.type = WaveRule::Type::Fixed;
                        rule.fixedMovement = AlienMovementType::SineWave;
                        rule.frequencyMul = 5.0f;
                        rule.frequencyAddPerLevel = 0.05f;
                        rule.vyAdd = 0.01f;
                        break;
                    case 3:
                        rule.type = WaveRule::Type::Fixed;
                        rule.fixedMovement = AlienMovementType::TogetherOne;
                        rule.setX = 0.0f;
                        rule.vyAdd = 0.01f;
                        break;
                    case 4:
                        rule.type = WaveRule::Type::RandomWeighted;
                        rule.weights = {
                                {AlienMovementType::LeftRight, 1},
                                {AlienMovementType::SnakeWave, 1},
                                {AlienMovementType::SineWave, 1},
                                {AlienMovementType::TogetherOne, 1},
                                {AlienMovementType::MySnakeWave, 1},
                                {AlienMovementType::JustGoDown, 1},
                        };
                        break;
                    default:
                        rule.type = WaveRule::Type::Fixed;
                        rule.fixedMovement = AlienMovementType::JustGoDown;
                        break;
                }
            }

            AlienMovementType movement = (rule.type == WaveRule::Type::RandomWeighted)
                                                 ? pickWeighted(rule.weights)
                                                 : rule.fixedMovement;

            alien.movementType = movement;
            applyRuleSetup(rule, alien, level_);

            world_.aliens.add(*e, alien);
            auto &pc = world_.render.add(*e, MainPushConstants{});
            pc.texturePos = 1;
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

    for (int slot = 0; slot < MAX_ALIENS; ++slot) {
        const ecs::EntityId e = alienEntities_[slot];
        if (!world_.registry.alive(e)) continue;
        Alien *alien = world_.aliens.tryGet(e);
        if (!alien || !alien->active) continue;

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

                const int row = slot / NUM_ALIENS_X;
                const int col = slot % NUM_ALIENS_X;

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
                if (alien->x > 0.85f) alien->x = 0.85f;
                if (alien->x < -0.85f) alien->x = -0.85f;

                alien->x += alienMoveSpeed_ * alienDirection_ * deltaTime;

                if (alien->x > 0.85f || alien->x < -0.85f) {
                    hitEdge = true;
                    if (hitEdge) {
                        alienDirection_ *= -1;
                        for (int s = 0; s < MAX_ALIENS; ++s) {
                            const ecs::EntityId ae = alienEntities_[s];
                            if (!world_.registry.alive(ae)) continue;
                            Alien *a = world_.aliens.tryGet(ae);
                            if (a && a->active) {
                                a->y -= 0.04f;
                            }
                        }
                    }
                }
                break;
        }
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
            b->y -= shipBulletSpeed_ * deltaTime;
        } else if (b->bulletType == BulletType::Alien) {
            b->y += alienBulletSpeed_ * deltaTime;
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

std::optional<ecs::EntityId> GameWorldManager::spawnBullet(BulletType type, const glm::vec2 &pos, const DamagePayload &payload) {
    const auto e = world_.registry.create();
    if (!e.has_value()) {
        return std::nullopt;
    }

    Bullet bullet{};
    bullet.active = true;
    bullet.bulletType = type;
    bullet.x = pos.x;
    bullet.y = pos.y;
    bullet.payload = payload;
    bullet.widthHeight = bulletWidthHeight_;

    world_.bullets.add(*e, bullet);
    world_.render.add(*e, MainPushConstants{});
    return *e;
}

void GameWorldManager::updateAndMaybeFire(bool isPlaying, float deltaTime) {
    if (!isPlaying) return;
    fireTimer_ += deltaTime;
    if (fireTimer_ <= fireInterval_) return;
    fireTimer_ = 0.0f;

    // pick a random active alien slot
    const uint32_t start = Util::getRandomUint(0, MAX_ALIENS - 1);
    for (uint32_t offset = 0; offset < MAX_ALIENS; ++offset) {
        const uint32_t slot = (start + offset) % MAX_ALIENS;
        const ecs::EntityId e = alienEntities_[slot];
        if (!world_.registry.alive(e)) continue;
        const Alien *alien = world_.aliens.tryGet(e);
        if (!alien || !alien->active) continue;

        (void) spawnBullet(BulletType::Alien, {alien->x, -alien->y}, makeKinetic(Util::getRandomFloat(10.0f, 30.0f)));
        break;
    }
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

    world_.registry.forEachAlive([&](ecs::EntityId be) {
        Bullet *bullet = world_.bullets.tryGet(be);
        if (!bullet || !bullet->active) return;

        // Ship bullets -> aliens
        if (bullet->bulletType == BulletType::Ship) {
            for (int slot = 0; slot < MAX_ALIENS; ++slot) {
                const ecs::EntityId ae = alienEntities_[slot];
                if (!world_.registry.alive(ae)) continue;
                Alien *alien = world_.aliens.tryGet(ae);
                if (!alien || !alien->active) continue;
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

        // Alien bullets -> ship
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
    for (int slot = 0; slot < MAX_ALIENS; ++slot) {
        const ecs::EntityId e = alienEntities_[slot];
        if (!world_.registry.alive(e)) continue;
        const Alien *a = world_.aliens.tryGet(e);
        if (a && a->active) return true;
    }
    return false;
}

bool GameWorldManager::hasAlienBelow(float threshold) const {
    for (int slot = 0; slot < MAX_ALIENS; ++slot) {
        const ecs::EntityId e = alienEntities_[slot];
        if (!world_.registry.alive(e)) continue;
        const Alien *a = world_.aliens.tryGet(e);
        if (a && a->active && a->y < threshold) return true;
    }
    return false;
}

void GameWorldManager::destroyEntity(ecs::EntityId entity) {
    if (!world_.registry.alive(entity)) return;

    world_.ships.remove(entity);
    world_.aliens.remove(entity);
    world_.bullets.remove(entity);
    world_.render.remove(entity);
    world_.registry.destroy(entity);
}

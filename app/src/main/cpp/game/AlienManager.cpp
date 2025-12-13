#include "AlienManager.h"

#include <algorithm>
#include <cstring>

#include <glm/vec3.hpp>

#include "Collision.h"
#include "Util.h"
#include "json/TinyJson.h"
#include "events/EventBus.h"
#include "platform/PlatformServices.h"

#include "ParticleSystem.h"

std::span<Alien> AlienManager::aliens() {
    return {world_.aliens.data(), world_.aliens.size()};
}

std::span<const Alien> AlienManager::aliens() const {
    return {world_.aliens.data(), world_.aliens.size()};
}

std::span<MainPushConstants> AlienManager::pushConstants() {
    return {world_.render.data(), world_.render.size()};
}

std::span<const MainPushConstants> AlienManager::pushConstants() const {
    return {world_.render.data(), world_.render.size()};
}

void AlienManager::resetMovement() {
    movementSystem_.reset();
    fireTimer_ = 0.0f;
}

std::optional<AlienMovementType> AlienManager::parseMovementType(const std::string &name) {
    if (name == "SnakeWave") return AlienMovementType::SnakeWave;
    if (name == "JustGoDown") return AlienMovementType::JustGoDown;
    if (name == "TogetherOne") return AlienMovementType::TogetherOne;
    if (name == "SineWave") return AlienMovementType::SineWave;
    if (name == "Circle") return AlienMovementType::Circle;
    if (name == "LeftRight") return AlienMovementType::LeftRight;
    if (name == "MySnakeWave") return AlienMovementType::MySnakeWave;
    return std::nullopt;
}

AlienMovementType AlienManager::pickWeighted(const std::unordered_map<AlienMovementType, uint32_t> &weights) {
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

void AlienManager::applyRuleSetup(const WaveRule &rule, Alien &alien, uint32_t level) {
    if (rule.setX.has_value()) {
        alien.x = *rule.setX;
    }

    if (rule.frequencyMul != 1.0f || rule.frequencyAddPerLevel != 0.0f) {
        alien.frequency = alien.baseFrequency * rule.frequencyMul + (static_cast<float>(level) * rule.frequencyAddPerLevel);
    }

    alien.vy += rule.vyAdd;
}

void AlienManager::loadConfig(IPlatformServices &platformServices) {
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

void AlienManager::initAliens() {
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
    if (wave_ >= 5) wave_ = 0; // reset wave

    world_.entities.clear();

    for (int y = 0; y < NUM_ALIENS_Y; ++y) {
        for (int x = 0; x < NUM_ALIENS_X; ++x) {
            int idx = y * NUM_ALIENS_X + x;
            (void) world_.entities.create();

            world_.aliens[idx].x = startX + x * dx;
            world_.aliens[idx].baseX = startX + x * dx;
            world_.aliens[idx].movementTimer = 0.0f;
            world_.aliens[idx].y = startY - y * dy;
            world_.aliens[idx].active = true;
            world_.aliens[idx].health.hull = 100.0f;
            world_.aliens[idx].health.dead = false;
            world_.aliens[idx].resistances = enemyRes;
            world_.aliens[idx].ailments = {};

            world_.aliens[idx].widthHeight = Util::getQuadWidthHeight(alienVerts, 6, {1.0f, 1.0f});
            world_.render[idx].texturePos = 1;

            if (numOfAliens_ >= 1) {
                AlienMovementType movement = AlienMovementType::JustGoDown;
                WaveRule rule{};
                if (!waveRules_.empty() && wave_ < waveRules_.size()) {
                    rule = waveRules_[wave_];
                } else {
                    // Fallback to old hard-coded rules if config is missing/invalid.
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

                if (rule.type == WaveRule::Type::RandomWeighted) {
                    movement = pickWeighted(rule.weights);
                } else {
                    movement = rule.fixedMovement;
                }

                world_.aliens[idx].movementType = movement;
                applyRuleSetup(rule, world_.aliens[idx], level_);
            }
        }
    }

    wave_++;
}

void AlienManager::update(float deltaTime) {
    movementSystem_.update(world_, deltaTime);
}

bool AlienManager::isShipBulletHittingAlien(const Alien &alien, const Bullet &bullet) {
    auto alienAABB = Collision::getAABB(alien.x, alien.y, alien.widthHeight[0], alien.widthHeight[1]);
    auto bulletAABB = Collision::getAABB(bullet.x, -bullet.y, bullet.widthHeight[0], bullet.widthHeight[1]);
    return Collision::isColliding(alienAABB, bulletAABB);
}

bool AlienManager::isAlienBulletHittingShip(const Ship &ship, const Bullet &bullet) {
    auto shipAABB = Collision::getAABB(ship.x, ship.y, ship.widthHeight[0], ship.widthHeight[1]);
    auto bulletAABB = Collision::getAABB(bullet.x, bullet.y, bullet.widthHeight[0], bullet.widthHeight[1]);
    return Collision::isColliding(shipAABB, bulletAABB);
}

void AlienManager::processCollisions(std::span<Bullet> bullets,
                                     Ship &ship,
                                     MainPushConstants &shipPushConstants,
                                     bool shieldActive,
                                     ParticleSystem &particleSystem,
                                     EventBus &eventBus) {
    for (auto &bullet: bullets) {
        if (!bullet.active) continue;

        for (uint32_t i = 0; i < MAX_ALIENS; i++) {
            if (!world_.aliens[i].active) continue;

            if (bullet.bulletType == BulletType::Ship && isShipBulletHittingAlien(world_.aliens[i], bullet)) {
                bullet.active = false;

                eventBus.publish(HitEvent{
                        .attacker = 0,
                        .target = i,
                        .payload = bullet.payload,
                        .hitWorldPos = glm::vec2(world_.aliens[i].x, world_.aliens[i].y),
                });

                world_.render[i].flashAmount = 1.0f;
                particleSystem.spawn(glm::vec3(world_.aliens[i].x, -world_.aliens[i].y, 1.0f), 5);
                break;
            }

            if (bullet.bulletType == BulletType::Alien && !shieldActive && isAlienBulletHittingShip(ship, bullet)) {
                bullet.active = false;

                eventBus.publish(HitEvent{
                        .attacker = i,
                        .target = ShipEntityId,
                        .payload = bullet.payload,
                        .hitWorldPos = glm::vec2(ship.x, ship.y),
                });

                shipPushConstants.flashAmount = 1.0f;
                particleSystem.spawn(glm::vec3(bullet.x, bullet.y, 0.0f), 10);
                break;
            }
        }
    }
}

std::optional<glm::vec2> AlienManager::updateAndMaybeFire(bool isPlaying, float deltaTime) {
    if (!isPlaying) return std::nullopt;

    fireTimer_ += deltaTime;
    if (fireTimer_ <= fireInterval_) return std::nullopt;

    fireTimer_ = 0.0f;

    const uint32_t idx = Util::getRandomUint(0, MAX_ALIENS - 1);
    const Alien &alien = world_.aliens[idx];
    if (!alien.active) return std::nullopt;

    return glm::vec2(alien.x, -alien.y);
}

bool AlienManager::hasActiveAliens() const {
    return std::any_of(world_.aliens.begin(), world_.aliens.end(), [](const Alien &alien) { return alien.active; });
}

bool AlienManager::hasAlienBelow(float threshold) const {
    return std::any_of(world_.aliens.begin(), world_.aliens.end(), [threshold](const Alien &alien) {
        return alien.active && alien.y < threshold;
    });
}

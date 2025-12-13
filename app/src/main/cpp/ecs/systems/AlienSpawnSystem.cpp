#include "AlienSpawnSystem.h"

#include <iterator>
#include <numeric>

#include "Util.h"
#include "ecs/worlds/GameWorld.h"
#include "game/GameWorldManager.h"

namespace ecs {

std::optional<AlienMovementType> AlienSpawnSystem::parseMovementType(const std::string &name) {
    if (name == "SnakeWave") return AlienMovementType::SnakeWave;
    if (name == "JustGoDown") return AlienMovementType::JustGoDown;
    if (name == "TogetherOne") return AlienMovementType::TogetherOne;
    if (name == "SineWave") return AlienMovementType::SineWave;
    if (name == "Circle") return AlienMovementType::Circle;
    if (name == "LeftRight") return AlienMovementType::LeftRight;
    if (name == "MySnakeWave") return AlienMovementType::MySnakeWave;
    return std::nullopt;
}

AlienMovementType AlienSpawnSystem::pickWeighted(const std::unordered_map<AlienMovementType, uint32_t> &weights) {
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

void AlienSpawnSystem::applyRuleSetup(const WaveRule &rule, Alien &alien, uint32_t level) {
    if (rule.setX.has_value()) {
        alien.x = *rule.setX;
    }

    if (rule.frequencyMul != 1.0f || rule.frequencyAddPerLevel != 0.0f) {
        alien.frequency = alien.baseFrequency * rule.frequencyMul + (static_cast<float>(level) * rule.frequencyAddPerLevel);
    }

    alien.vy += rule.vyAdd;
}

void AlienSpawnSystem::applyModifiers(const PrefabLibrary &library, const std::vector<std::string> &mods, Alien &alien) {
    for (const auto &name : mods) {
        const auto *mod = library.modifier(name);
        if (!mod) continue;
        for (size_t i = 0; i < std::size(alien.resistances.byType); ++i) {
            alien.resistances.byType[i] += mod->resistanceDelta.byType[i];
        }
        alien.armor.flatReduction += mod->armorDelta.flatReduction;
        alien.ailments = mod->ailments;
    }
}

void AlienSpawnSystem::update(GameWorldManager &manager, float, bool isPlaying) {
    if (!isPlaying) return;

    const auto settingsId = manager.settingsEntity();
    if (!settingsId.has_value()) return;

    auto &world = manager.world();
    auto &wavePool = world.pool<WaveSettings>();
    auto *settings = wavePool.tryGet(*settingsId);
    if (!settings || !settings->needsSpawn) return;

    if (settings->waves.empty()) {
        WaveDefinition fallback{};
        settings->waves.push_back(fallback);
    }

    const WaveDefinition &wave = settings->waves[settings->waveIndex % settings->waves.size()];
    settings->activeAlienBulletPrefab = wave.bulletPrefab;

    auto &aliens = world.pool<Alien>();
    auto &render = world.pool<MainPushConstants>();

    for (uint32_t r = 0; r < wave.rows; ++r) {
        for (uint32_t c = 0; c < wave.cols; ++c) {
            const auto e = world.registry.create();
            if (!e.has_value()) continue;

            Alien alien = manager.prefabs().alien(wave.prefabName).alien;
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
            applyRuleSetup(rule, alien, settings->level);
            applyModifiers(manager.prefabs(), wave.modifiers, alien);

            const auto &prefab = manager.prefabs().alien(wave.prefabName);
            auto &pc = render.add(*e, prefab.render);
            pc.texturePos = prefab.render.texturePos;

            aliens.add(*e, alien);
        }
    }

    settings->waveIndex++;
    settings->needsSpawn = false;

    auto &movement = world.pool<DirectionalMovement>();
    movement.add(*settingsId, DirectionalMovement{});

    auto &cooldown = world.pool<FireCooldown>();
    cooldown.add(*settingsId, FireCooldown{});
}

} // namespace ecs


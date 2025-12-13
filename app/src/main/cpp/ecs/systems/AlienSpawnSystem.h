#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include "ecs/components/GameplayComponents.h"

class GameWorldManager;

namespace ecs {

class AlienSpawnSystem {
public:
    void update(GameWorldManager &manager, float deltaTime, bool isPlaying);

    static std::optional<AlienMovementType> parseMovementType(const std::string &name);
    static AlienMovementType pickWeighted(const std::unordered_map<AlienMovementType, uint32_t> &weights);
    static void applyRuleSetup(const WaveRule &rule, Alien &alien, uint32_t level);
    static void applyModifiers(const PrefabLibrary &library, const std::vector<std::string> &mods, Alien &alien);
};

} // namespace ecs


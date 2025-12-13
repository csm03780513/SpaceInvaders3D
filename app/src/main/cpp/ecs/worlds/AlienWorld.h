#pragma once

#include <array>

#include "GameConstants.h"
#include "GameObjectData.h"
#include "ecs/components/CombatComponents.h"
#include "ecs/registry/FixedEntityPool.h"

namespace ecs {

struct AlienWorld {
    FixedEntityPool<MAX_ALIENS> entities{};
    std::array<Alien, MAX_ALIENS> aliens{};
    std::array<MainPushConstants, MAX_ALIENS> render{};
};

} // namespace ecs


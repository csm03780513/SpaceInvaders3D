#pragma once

#include <array>

#include "GameConstants.h"
#include "ecs/components/CombatComponents.h"
#include "ecs/registry/FixedEntityPool.h"

namespace ecs {

struct CombatWorld {
    Ship ship{};

    FixedEntityPool<MAX_BULLETS> bulletEntities{};
    std::array<Bullet, MAX_BULLETS> bullets{};
};

} // namespace ecs


#pragma once

#include "GameConstants.h"
#include "ecs/components/CombatComponents.h"
#include "ecs/registry/ComponentPool.h"
#include "ecs/registry/EntityRegistry.h"

namespace ecs {

static constexpr size_t MAX_GAME_ENTITIES = 256;

struct GameWorld {
    EntityRegistry<MAX_GAME_ENTITIES> registry{};

    ComponentPool<Ship, MAX_GAME_ENTITIES> ships{};
    ComponentPool<Alien, MAX_GAME_ENTITIES> aliens{};
    ComponentPool<Bullet, MAX_GAME_ENTITIES> bullets{};
    ComponentPool<MainPushConstants, MAX_GAME_ENTITIES> render{};

    EntityId shipEntity = 0;
};

} // namespace ecs


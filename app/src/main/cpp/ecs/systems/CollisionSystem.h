#pragma once

class EventBus;
class ParticleSystem;
class GameWorldManager;

namespace ecs {

class CollisionSystem {
public:
    void update(GameWorldManager &manager,
                ParticleSystem &particleSystem,
                EventBus &eventBus,
                bool shieldActive) const;
};

} // namespace ecs


#pragma once

class GameWorldManager;

namespace ecs {

class FiringSystem {
public:
    void update(GameWorldManager &manager, float deltaTime, bool isPlaying);
};

} // namespace ecs


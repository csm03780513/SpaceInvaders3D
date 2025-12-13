#pragma once

class GameWorldManager;

namespace ecs {

class BulletSpawnSystem {
public:
    void update(GameWorldManager &manager, bool isPlaying) const;
};

} // namespace ecs


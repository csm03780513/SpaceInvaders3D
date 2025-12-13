#pragma once

#include <cmath>

#include "GameConstants.h"
#include "ecs/worlds/AlienWorld.h"

namespace ecs {

class AlienMovementSystem {
public:
    void reset() {
        moveSpeed_ = 0.3f;
        direction_ = 1.0f;
    }

    void update(AlienWorld &world, float deltaTime) {
        bool hitEdge = false;

        world.entities.forEachAlive([&](EntityId id) {
            Alien &alien = world.aliens[id];
            if (!alien.active) return;

            world.render[id].flashAmount -= deltaTime * 5.0f;
            if (world.render[id].flashAmount < 0.0f) world.render[id].flashAmount = 0.0f;

            switch (alien.movementType) {
                case TogetherOne:
                    alien.y -= alien.vy * deltaTime;
                    break;
                case SineWave:
                    alien.movementTimer += deltaTime;
                    alien.x = alien.baseX + alien.amplitude * std::sin(alien.movementTimer * alien.frequency);
                    alien.y -= alien.vy * deltaTime;
                    break;
                case MySnakeWave:
                    alien.movementTimer += deltaTime;
                    alien.x = std::sin((alien.movementTimer + alien.baseX) * alien.frequency);
                    alien.y -= alien.vy * deltaTime;
                    break;
                case SnakeWave: {
                    alien.movementTimer += deltaTime;

                    const int row = static_cast<int>(id) / NUM_ALIENS_X;
                    const int col = static_cast<int>(id) % NUM_ALIENS_X;

                    const float basePhase = alien.movementTimer * alien.frequency;
                    const float rowPhase = row * 0.45f;
                    const float colPhase = col * 0.25f;

                    const float primaryWave = std::sin(basePhase + rowPhase);
                    const float secondaryWave = std::sin(basePhase * 0.65f + colPhase);

                    alien.x = alien.baseX + alien.amplitude * (0.75f * primaryWave + 0.35f * secondaryWave);

                    const float verticalBobVelocity = std::cos(basePhase + rowPhase) * alien.frequency * 0.12f;
                    alien.y -= alien.vy * deltaTime;
                    alien.y += verticalBobVelocity * deltaTime;

                    alien.x += 0.05f * std::sin(basePhase * 1.8f + colPhase + rowPhase);
                    break;
                }
                case JustGoDown:
                    alien.y -= alien.vy * deltaTime;
                    break;
                case Circle:
                    break;
                case LeftRight:
                    if (alien.x > 0.85f) alien.x = 0.85f;
                    if (alien.x < -0.85f) alien.x = -0.85f;

                    alien.x += moveSpeed_ * direction_ * deltaTime;

                    if (alien.x > 0.85f || alien.x < -0.85f) {
                        hitEdge = true;
                        if (hitEdge) {
                            direction_ *= -1;
                            for (auto &a : world.aliens) {
                                if (a.active) a.y -= 0.04f;
                            }
                        }
                    }
                    break;
            }
        });
    }

private:
    float moveSpeed_ = 0.3f;
    float direction_ = 1.0f;
};

} // namespace ecs


#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/vec2.hpp>

#include "GameConstants.h"

struct WaveRule {
    enum class Type { Fixed, RandomWeighted };
    Type type = Type::Fixed;
    AlienMovementType fixedMovement = AlienMovementType::LeftRight;
    std::unordered_map<AlienMovementType, uint32_t> weights{};

    float frequencyMul = 1.0f;
    float frequencyAddPerLevel = 0.0f;
    float vyAdd = 0.0f;
    std::optional<float> setX{};
};

struct WaveDefinition {
    std::string name{"wave"};
    std::string prefabName{"grunt"};
    std::string bulletPrefab{"alien_primary"};
    uint32_t rows = NUM_ALIENS_Y;
    uint32_t cols = NUM_ALIENS_X;
    glm::vec2 start{-0.7f, 0.8f};
    glm::vec2 spacing{0.2f, 0.15f};
    WaveRule rule{};
    std::vector<std::string> modifiers{};
};

struct WaveSettings {
    std::vector<WaveDefinition> waves{};
    uint32_t waveIndex = 0;
    uint32_t level = 0;
    std::string activeAlienBulletPrefab{"alien_primary"};
    bool needsSpawn = false;
};

struct FireCooldown {
    float timer = 0.0f;
    float interval = 1.0f;
};

struct DirectionalMovement {
    float speed = 0.3f;
    float direction = 1.0f;
};


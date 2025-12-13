//
// Created by carlo on 25/10/2025.
//

#ifndef SPACEINVADERS3D_COMBATCOMPONENTS_H
#define SPACEINVADERS3D_COMBATCOMPONENTS_H


#include "mechanics/Damage.h"

struct Health {
    float shield   = 0.0f;     // first layer
    float hull     = 100.0f;   // second layer
    float maxShield= 0.0f;
    float maxHull  = 100.0f;
    bool  dead     = false;
};

// % reductions per type (0.25 = 25% less damage taken)
struct Resistances {
    float byType[static_cast<int>(DamageType::Cosmic)+1] = {0};
    // optional “negative resistance” allowed (e.g., -0.25 = 25% more dmg)
};

// Optional armor (applies to **Kinetic** on hull only)
struct Armor {
    float flatReduction = 0.0f;  // subtract from post-resist dmg (min 0)
};

// Running ailments / temporary effects
struct Ailments {
    // Fire/Poison/Radiation DoTs; stored as DPS so we can be dt-independent.
    float burnDps     = 0.0f;
    float poisonDps   = 0.0f;
    float radiationDps= 0.0f;

    // Cold slow (0..1) and time-left
    float slowFactor  = 0.0f;   // 0.35 = 35% speed reduction
    float slowTtl     = 0.0f;

    // Lightning stun
    bool  stunned     = false;
    float stunTtl     = 0.0f;

    // Defense shred (from Radiation) applies to resistances.
    float shredAmount = 0.0f;   // e.g., 0.15 reduces all res by 15%
    float shredTtl    = 0.0f;

    // Time accumulator to throttle DOT popups.
    float dotPopupTimer = 0.0f;
};

// How to convert hits into ailments; tune these centrally.
struct AilmentRules {
    // thresholds are post-resist single-slice hit amounts
    float igniteMin     = 5.0f;                // Fire
    float igniteDpsMult = 0.4f;                // DPS = dmg * mult, 2s base
    float igniteBaseT   = 2.0f;

    float shockMin      = 5.0f;                // Lightning stun
    float shockStunT    = 0.2f;                // seconds

    float chillMin      = 3.0f;                // Cold
    float chillSlow     = 0.30f;               // 30%
    float chillT        = 2.5f;

    float poisonStackMin= 2.0f;                // Poison
    float poisonDpsMult = 0.2f;
    float poisonT       = 3.0f;

    float radMin        = 4.0f;                // Radiation
    float radDpsMult    = 0.15f;
    float radShred      = 0.10f;               // -10% all res
    float radT          = 4.0f;

    // Plasma helper split if you prefer to auto-derive
    float plasmaFireRatio = 0.5f;              // remainder is Lightning
};

// Optional shields rule: some types interact differently
struct ShieldRules {
    bool kineticHalfOnShield = false; // e.g., kinetic less effective vs shields
    bool darkMatterIgnoresArmor = true;
};

enum class BulletType {
    Ship,
    Alien
};

struct Bullet {
    float x{}, y{};
    bool active{};
    std::array<float, 2> widthHeight{};
    BulletType bulletType{};
    static constexpr float size = 0.05f * 0.5f; // half alien
    float speed{1.0f};
    DamagePayload payload{};
};

struct Ship {
    float x{}, y{};
    float color[3]{};
    std::array<float, 2> widthHeight{};
    float size{0.1f};

    Health health;
    Armor armor;
    Resistances resistances;
    Ailments ailments;
};

struct Alien {
    float x{}, y{}, vx{0.1f}, vy{0.02f};
    float movementTimer = 0.0f;   // Used for sine phase
    float amplitude = 0.5f;      // Sine wave width (tune for look)
    float frequency = 0.1f;       // Sine wave speed
    float baseFrequency = 0.1f;
    float baseX = 0.0f;
    AlienMovementType movementType{AlienMovementType::LeftRight};
    bool active{};
    std::array<float, 2> widthHeight{};
    uint16_t spawnRow = 0;
    uint16_t spawnCol = 0;

    Health health;
    Armor armor;
    Resistances resistances;
    Ailments ailments;
};

class CombatComponents {

};


#endif //SPACEINVADERS3D_COMBATCOMPONENTS_H

//
// Created by carlo on 26/10/2025.
//

#ifndef SPACEINVADERS3D_DAMAGE_H
#define SPACEINVADERS3D_DAMAGE_H

#include "../GameObjectData.h"

enum class DamageType : uint8_t {
    Kinetic,    // raw impact
    Fire,       // burn DoT
    Lightning,  // chain/stun chance
    Cold,       // slow
    Poison,     // stacking DoT
    Radiation,  // aura / defense shred
    Plasma,     // split Fire/Lightning
    DarkMatter, // bypass armor, hit shields normally
    Cosmic      // exotic (boss / gravity distort)
};

// A single typed damage contribution.
struct DamageSlice {
    DamageType type;
    float amount;      // base, pre-resistance
};

// Full payload supports mixed damage (e.g., Plasma = 50/50 Fire/Lightning)
struct DamagePayload {
    std::vector<DamageSlice> slices;
    float critChance = 0.0f;   // 0..1
    float critMult   = 1.5f;   // 1.0 = no crit
};

// Weapon factories
inline DamagePayload makeKinetic(float dmg, float crit=0.1f, float mult=1.8f) {
    return { { {DamageType::Kinetic, dmg} }, crit, mult };
}

inline DamagePayload makePlasma(float dmg) {
    // explicitly split for clarity
    return { { {DamageType::Fire, dmg*0.5f}, {DamageType::Lightning, dmg*0.5f} }, 0.05f, 1.5f };
}

inline DamagePayload makeCryo(float dmg) {
    return { { {DamageType::Cold, dmg} }, 0.0f, 1.0f };
}

inline DamagePayload makeToxin(float dpsHitEquiv) {
    return { { {DamageType::Poison, dpsHitEquiv} }, 0.0f, 1.0f };
}

class Damage {

};


#endif //SPACEINVADERS3D_DAMAGE_H

//
// Created by carlo on 26/10/2025.
//

#ifndef SPACEINVADERS3D_COMBATEVENTS_H
#define SPACEINVADERS3D_COMBATEVENTS_H

// CombatEvents.hpp
#pragma once
#include <cstdint>
#include <string>
#include <glm/vec2.hpp>
#include "../../mechanics/Damage.h"

inline constexpr uint32_t ShipEntityId = 999u;

struct HitEvent {
    uint32_t attacker;
    uint32_t target;
    DamagePayload payload;
    glm::vec2 hitWorldPos;
};

struct DamageAppliedEvent {
    uint32_t target;
    float totalDamage;        // post-mitigation
    bool  killed;
    glm::vec2 worldPos;
};

struct DamagePopupSpawned {
    glm::vec2 worldPos;
    std::string text;  // e.g., "128", "CRIT 220", "Burn 12"
    glm::vec4 rgba;     // 0xAARRGGBB (map by DamageType)
    float ttl;         // seconds
    float riseSpeed;   // units/sec
    float startScale {0.0025f};
};


class CombatEvents {

};


#endif //SPACEINVADERS3D_COMBATEVENTS_H

// AilmentSystem.hpp
#pragma once

#include "../components/CombatComponents.h"
#include "../events/CombatEvents.h"
#include <algorithm>
#include <vector>
#include <sstream>

struct AilmentSystem {
    void tick(float dt, uint32_t entity, Health &hp, Ailments &al, const glm::vec2& worldPos, std::vector<DamagePopupSpawned> &outPopups) const {
        if (hp.dead) return;
        const float totalDotDps = al.burnDps + al.poisonDps + al.radiationDps;
        float dotDmg = totalDotDps * dt;
        if (dotDmg > 0.0f) {
            const float toShield = std::min(hp.shield, dotDmg);
            hp.shield -= toShield;
            dotDmg -= toShield;
            if (dotDmg > 0.0f) {
                hp.hull = std::max(0.0f, hp.hull - dotDmg);
            }
            // Space out DOT popups so we show at most one every 0.2 seconds.
            constexpr float popupPeriod = 0.3f;
            al.dotPopupTimer += dt;
            while (al.dotPopupTimer >= popupPeriod) {
                al.dotPopupTimer -= popupPeriod;
                std::ostringstream oss;
                oss << static_cast<int>(std::round(totalDotDps));
                outPopups.push_back(DamagePopupSpawned{
                        /*worldPos*/worldPos, oss.str(),
                        glm::vec4(0.8667f, 0.3333f, 0.6667f, 1.0f) /*mixed*/, 0.5f, 0.8f
                });
            }

            if (hp.hull <= 0.0f) hp.dead = true;
        } else {
            al.dotPopupTimer = 0.0f;
        }

        // Decay timers
        if (al.slowTtl > 0.f) {
            al.slowTtl -= dt;
            if (al.slowTtl <= 0.f) al.slowFactor = 0.0f;
        }
        if (al.stunTtl > 0.f) {
            al.stunTtl -= dt;
            if (al.stunTtl <= 0.f) al.stunned = false;
        }
        if (al.shredTtl > 0.f) {
            al.shredTtl -= dt;
            if (al.shredTtl <= 0.f) al.shredAmount = 0.0f;
        }

        // Optional natural decay for stacked poison to avoid infinite build-up
        const float poisonDecay = 0.15f; // per second
        al.poisonDps = std::max(0.0f, al.poisonDps - poisonDecay * dt);

        // Burn/Rad also fade when not refreshed
        const float burnDecay = 0.20f, radDecay = 0.12f;
        al.burnDps = std::max(0.0f, al.burnDps - burnDecay * dt);
        al.radiationDps = std::max(0.0f, al.radiationDps - radDecay * dt);
    }
};

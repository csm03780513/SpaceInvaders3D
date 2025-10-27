//
// Created by carlo on 25/10/2025.
//

#ifndef SPACEINVADERS3D_DAMAGESYSTEM_H
#define SPACEINVADERS3D_DAMAGESYSTEM_H


// DamageSystem.hpp
#pragma once

#include <algorithm>
#include <sstream>
#include "../components/CombatComponents.h"
#include "../events/CombatEvents.h"
#include "../../mechanics/DamageViz.h"

struct DamageContext {
    const AilmentRules *ailRules = nullptr;
    const ShieldRules *shRules = nullptr;
};

// Helper: apply resistances (+ optional shred) and return mitigated amount
inline float applyResists(const Resistances &res, float shred, DamageType t, float amt) {
    const int idx = static_cast<int>(t);
    const float r = std::clamp(res.byType[idx] - shred, -0.90f, 0.90f);
    return std::max(0.0f, amt * (1.0f - r));
}

inline bool rollCrit(float chance01) {
    if (chance01 <= 0.0f) return false;
    // NOTE: replace with your RNG
    const float r = (float) rand() / (float) RAND_MAX;
    return r < chance01;
}

struct DamageSystem {
    DamageContext ctx;

    // Call this when collision detects a bullet hit → enqueue HitEvent.
    // Then, in your gameplay tick, process all HitEvents here.
    void apply(
            uint32_t target,
            Health &hp,
            const Resistances &res,
            const Armor *armorOpt,
            Ailments &al,
            const HitEvent &e,
            std::vector<DamageAppliedEvent> &outApplied,
            std::vector<DamagePopupSpawned> &outPopups
    ) const {
        if (hp.dead) return;
        float totalApplied = 0.0f;
        float shred = al.shredAmount; // radiation shred currently on target

        // Compute crit (single roll for the whole payload for readability)
        const bool crit = rollCrit(e.payload.critChance);
        const float critMult = crit ? e.payload.critMult : 1.0f;

        // Sum per-slice post-mitigation and apply to shield then hull.
        float remainingToApply = 0.0f;
        for (const auto &s: e.payload.slices) {
            float amt = s.amount * critMult;

            // Plasma convenience: if you want to build it on-the-fly
            // (Alternatively: author Plasma as two slices in the bullet data)
            DamageType t = s.type;

            // Shield interaction tweaks
            if (t == DamageType::Kinetic && ctx.shRules && ctx.shRules->kineticHalfOnShield && hp.shield > 0.f) {
                amt *= 0.5f;
            }

            float postRes = applyResists(res, shred, t, amt);

            // Armor applies to **Kinetic** on hull only (after shield is gone)
            bool applyArmor = (t == DamageType::Kinetic) &&
                              (!ctx.shRules || !ctx.shRules->darkMatterIgnoresArmor);
            remainingToApply += postRes;

            // Spawn a small per-slice popup (optional). You may prefer one merged popup below.
            {
                std::ostringstream oss;
                if (crit) oss << "CRIT ";
                oss << (int) std::round(postRes);
                outPopups.push_back(DamagePopupSpawned{
                        e.hitWorldPos, oss.str(), crit ? glm::vec4{1.0f, 0.0f, 0.0f, 1.0f} : colorFor(t), 0.6f, 1.2f
                });
            }

            // Ailments (based on **postRes** of that slice)
            applyAilmentsFromHit(t, postRes, al);
        }

        // Apply to shields first
        float dmg = remainingToApply;

        if (hp.shield > 0.f && dmg > 0.f) {
            const float toShield = std::min(hp.shield, dmg);
            hp.shield -= toShield;
            dmg -= toShield;
        }

        // Then to hull (armor on kinetic)
        if (dmg > 0.f) {
            float hullDmg = dmg;
            // Simple armor: flat reduction to the portion that is kinetic.
            // If you want exactness per-slice, move armor earlier and track kinetic share.
            if (const Armor *a = armorOpt) {
                hullDmg = std::max(0.0f, hullDmg - a->flatReduction);
            }
            hp.hull = std::max(0.0f, hp.hull - hullDmg);
            totalApplied += hullDmg;
        }

        if (hp.hull <= 0.0f) {
            hp.dead = true;
        }

        outApplied.push_back(DamageAppliedEvent{
                .target = target,
                .totalDamage = totalApplied,
                .killed = hp.dead,
                .worldPos = e.hitWorldPos
        });
    }

private:
    void applyAilmentsFromHit(DamageType t, float postResHit, Ailments &al) const {
        if (!ctx.ailRules) {
            return;
        }
        const auto &R = *ctx.ailRules;

        switch (t) {
            case DamageType::Fire:
                if (postResHit >= R.igniteMin) {
                    al.burnDps = std::max(al.burnDps, postResHit * R.igniteDpsMult / R.igniteBaseT);
                    // refresh duration via tiny trick: store in slowTtl for reuse? Prefer separate ttl fields if you like.
                    // For brevity, we’ll just make poison/radiation/slow/stun carry their own TTls,
                    // while burns reuse poisonT (not ideal in prod—split if needed).
                }
                break;
            case DamageType::Lightning:
                if (postResHit >= R.shockMin) {
                    al.stunned = true;
                    al.stunTtl = std::max(al.stunTtl, R.shockStunT);
                }
                break;
            case DamageType::Cold:
                if (postResHit >= R.chillMin) {
                    al.slowFactor = std::max(al.slowFactor, R.chillSlow);
                    al.slowTtl = std::max(al.slowTtl, R.chillT);
                }
                break;
            case DamageType::Poison:
                if (postResHit >= R.poisonStackMin) {
                    al.poisonDps += (postResHit * R.poisonDpsMult / R.poisonT); // stacking
                }
                break;
            case DamageType::Radiation:
                if (postResHit >= R.radMin) {
                    al.radiationDps = std::max(al.radiationDps, postResHit * R.radDpsMult / R.radT);
                    al.shredAmount = std::max(al.shredAmount, R.radShred);
                    al.shredTtl = std::max(al.shredTtl, R.radT);
                }
                break;
            default:
                break;
        }
    }
};


#endif //SPACEINVADERS3D_DAMAGESYSTEM_H

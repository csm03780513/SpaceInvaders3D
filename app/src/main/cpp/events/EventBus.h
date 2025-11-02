#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include <algorithm>

#include "../ecs/events/CombatEvents.h"

class EventBus {
public:
    using HitHandler = std::function<void(const HitEvent &)>;
    using DamageAppliedHandler = std::function<void(const DamageAppliedEvent &)>;
    using DamagePopupHandler = std::function<void(const DamagePopupSpawned &)>;

    [[nodiscard]] uint32_t subscribeHit(const HitHandler &handler);
    void unsubscribeHit(uint32_t id);
    void publish(const HitEvent &event) const;

    [[nodiscard]] uint32_t subscribeDamageApplied(const DamageAppliedHandler &handler);
    void unsubscribeDamageApplied(uint32_t id);
    void publish(const DamageAppliedEvent &event) const;

    [[nodiscard]] uint32_t subscribeDamagePopup(const DamagePopupHandler &handler);
    void unsubscribeDamagePopup(uint32_t id);
    void publish(const DamagePopupSpawned &event) const;

private:
    template <typename Entry>
    static void removeById(std::vector<Entry> &entries, uint32_t id) {
        entries.erase(std::remove_if(entries.begin(), entries.end(), [id](const Entry &entry) {
            return entry.id == id;
        }), entries.end());
    }

    struct HitSubscription {
        uint32_t id;
        HitHandler handler;
    };
    struct DamageAppliedSubscription {
        uint32_t id;
        DamageAppliedHandler handler;
    };
    struct DamagePopupSubscription {
        uint32_t id;
        DamagePopupHandler handler;
    };

    uint32_t nextId_ = 1;
    std::vector<HitSubscription> hitSubscribers_;
    std::vector<DamageAppliedSubscription> damageAppliedSubscribers_;
    std::vector<DamagePopupSubscription> damagePopupSubscribers_;
};

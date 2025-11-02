#include "EventBus.h"

#include <algorithm>

uint32_t EventBus::subscribeHit(const HitHandler &handler) {
    const uint32_t id = nextId_++;
    hitSubscribers_.push_back({id, handler});
    return id;
}

void EventBus::unsubscribeHit(uint32_t id) {
    removeById(hitSubscribers_, id);
}

void EventBus::publish(const HitEvent &event) const {
    auto subscribers = hitSubscribers_;
    for (const auto &subscriber : subscribers) {
        if (subscriber.handler) {
            subscriber.handler(event);
        }
    }
}

uint32_t EventBus::subscribeDamageApplied(const DamageAppliedHandler &handler) {
    const uint32_t id = nextId_++;
    damageAppliedSubscribers_.push_back({id, handler});
    return id;
}

void EventBus::unsubscribeDamageApplied(uint32_t id) {
    removeById(damageAppliedSubscribers_, id);
}

void EventBus::publish(const DamageAppliedEvent &event) const {
    auto subscribers = damageAppliedSubscribers_;
    for (const auto &subscriber : subscribers) {
        if (subscriber.handler) {
            subscriber.handler(event);
        }
    }
}

uint32_t EventBus::subscribeDamagePopup(const DamagePopupHandler &handler) {
    const uint32_t id = nextId_++;
    damagePopupSubscribers_.push_back({id, handler});
    return id;
}

void EventBus::unsubscribeDamagePopup(uint32_t id) {
    removeById(damagePopupSubscribers_, id);
}

void EventBus::publish(const DamagePopupSpawned &event) const {
    auto subscribers = damagePopupSubscribers_;
    for (const auto &subscriber : subscribers) {
        if (subscriber.handler) {
            subscriber.handler(event);
        }
    }
}

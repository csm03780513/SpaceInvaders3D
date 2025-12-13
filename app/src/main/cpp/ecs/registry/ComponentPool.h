#pragma once

#include <array>
#include <cstdint>

#include "ecs/registry/EntityRegistry.h"

namespace ecs {

template <typename T, size_t MaxEntities>
class ComponentPool {
public:
    void reset() {
        has_.fill(false);
    }

    [[nodiscard]] bool has(EntityId id) const {
        const uint16_t idx = entityIndex(id);
        return idx < MaxEntities && has_[idx];
    }

    T &add(EntityId id, const T &value = T{}) {
        const uint16_t idx = entityIndex(id);
        has_[idx] = true;
        data_[idx] = value;
        return data_[idx];
    }

    void remove(EntityId id) {
        const uint16_t idx = entityIndex(id);
        if (idx >= MaxEntities) return;
        has_[idx] = false;
    }

    [[nodiscard]] T *tryGet(EntityId id) {
        const uint16_t idx = entityIndex(id);
        if (idx >= MaxEntities) return nullptr;
        if (!has_[idx]) return nullptr;
        return &data_[idx];
    }

    [[nodiscard]] const T *tryGet(EntityId id) const {
        const uint16_t idx = entityIndex(id);
        if (idx >= MaxEntities) return nullptr;
        if (!has_[idx]) return nullptr;
        return &data_[idx];
    }

    [[nodiscard]] T &get(EntityId id) {
        return data_[entityIndex(id)];
    }

    [[nodiscard]] const T &get(EntityId id) const {
        return data_[entityIndex(id)];
    }

private:
    std::array<T, MaxEntities> data_{};
    std::array<bool, MaxEntities> has_{};
};

} // namespace ecs


#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace ecs {

using EntityId = uint32_t;

template <size_t MaxEntities>
class FixedEntityPool {
public:
    FixedEntityPool() {
        for (size_t i = 0; i < MaxEntities; ++i) {
            alive_[i] = false;
        }
    }

    [[nodiscard]] std::optional<EntityId> create() {
        for (EntityId id = 0; id < MaxEntities; ++id) {
            if (!alive_[id]) {
                alive_[id] = true;
                return id;
            }
        }
        return std::nullopt;
    }

    void destroy(EntityId id) {
        if (id >= MaxEntities) return;
        alive_[id] = false;
    }

    void clear() {
        for (size_t i = 0; i < MaxEntities; ++i) {
            alive_[i] = false;
        }
    }

    [[nodiscard]] bool alive(EntityId id) const {
        if (id >= MaxEntities) return false;
        return alive_[id];
    }

    template <typename Fn>
    void forEachAlive(Fn &&fn) {
        for (EntityId id = 0; id < MaxEntities; ++id) {
            if (alive_[id]) {
                fn(id);
            }
        }
    }

private:
    std::array<bool, MaxEntities> alive_{};
};

} // namespace ecs


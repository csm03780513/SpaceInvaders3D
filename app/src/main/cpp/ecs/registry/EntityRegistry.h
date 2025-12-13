#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace ecs {

using EntityId = uint32_t;

inline constexpr uint32_t kEntityIndexBits = 16;
inline constexpr uint32_t kEntityIndexMask = (1u << kEntityIndexBits) - 1u;

[[nodiscard]] inline uint16_t entityIndex(EntityId id) {
    return static_cast<uint16_t>(id & kEntityIndexMask);
}

[[nodiscard]] inline uint16_t entityGeneration(EntityId id) {
    return static_cast<uint16_t>((id >> kEntityIndexBits) & kEntityIndexMask);
}

[[nodiscard]] inline EntityId makeEntityId(uint16_t index, uint16_t generation) {
    return (static_cast<EntityId>(generation) << kEntityIndexBits) | static_cast<EntityId>(index);
}

template <size_t MaxEntities>
class EntityRegistry {
public:
    EntityRegistry() {
        reset();
    }

    void reset() {
        alive_.fill(false);
        generations_.fill(1);
        freeCount_ = MaxEntities;
        for (size_t i = 0; i < MaxEntities; ++i) {
            free_[i] = static_cast<uint16_t>(MaxEntities - 1 - i);
        }
    }

    [[nodiscard]] std::optional<EntityId> create() {
        if (freeCount_ == 0) return std::nullopt;
        const uint16_t index = free_[--freeCount_];
        alive_[index] = true;
        return makeEntityId(index, generations_[index]);
    }

    void destroy(EntityId id) {
        const uint16_t idx = entityIndex(id);
        if (idx >= MaxEntities) return;
        if (!alive_[idx]) return;
        if (generations_[idx] != entityGeneration(id)) return;

        alive_[idx] = false;
        generations_[idx] = static_cast<uint16_t>(generations_[idx] + 1);
        free_[freeCount_++] = idx;
    }

    [[nodiscard]] bool alive(EntityId id) const {
        const uint16_t idx = entityIndex(id);
        if (idx >= MaxEntities) return false;
        if (!alive_[idx]) return false;
        return generations_[idx] == entityGeneration(id);
    }

    template <typename Fn>
    void forEachAlive(Fn &&fn) const {
        for (uint16_t idx = 0; idx < MaxEntities; ++idx) {
            if (!alive_[idx]) continue;
            fn(makeEntityId(idx, generations_[idx]));
        }
    }

private:
    std::array<bool, MaxEntities> alive_{};
    std::array<uint16_t, MaxEntities> generations_{};
    std::array<uint16_t, MaxEntities> free_{};
    size_t freeCount_ = 0;
};

} // namespace ecs


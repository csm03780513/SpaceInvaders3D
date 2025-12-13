#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>

#include "GameConstants.h"
#include "ecs/registry/ComponentPool.h"
#include "ecs/registry/EntityRegistry.h"

namespace ecs {

static constexpr size_t MAX_GAME_ENTITIES = 256;

struct GameWorld {
    EntityRegistry<MAX_GAME_ENTITIES> registry{};

    template <typename Component>
    [[nodiscard]] ComponentPool<Component, MAX_GAME_ENTITIES> &pool() const {
        const std::type_index key{typeid(Component)};
        auto it = pools_.find(key);
        if (it == pools_.end()) {
            auto wrapper = std::make_unique<PoolModel<Component>>();
            auto *ptr = wrapper.get();
            pools_.emplace(key, std::move(wrapper));
            return ptr->pool;
        }
        return static_cast<PoolModel<Component> &>(*it->second).pool;
    }

    void reset() {
        registry.reset();
        for (auto &pool : pools_) {
            pool.second->reset();
        }
    }

private:
    struct PoolConcept {
        virtual ~PoolConcept() = default;
        virtual void reset() = 0;
    };

    template <typename Component>
    struct PoolModel : PoolConcept {
        ComponentPool<Component, MAX_GAME_ENTITIES> pool{};
        void reset() override { pool.reset(); }
    };

    mutable std::unordered_map<std::type_index, std::unique_ptr<PoolConcept>> pools_{};
};

} // namespace ecs


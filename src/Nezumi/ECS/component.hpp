#pragma once

#include "ecs/types.hpp"
#include "utils/assert.hpp"

#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace Game::ECS {

// =============================================================================
// Component Types
// =============================================================================

using ComponentID   = uint8_t;
using ComponentMask = mask_t;
using ComponentSize = uint16_t;

constexpr const size_t COMPONENT_CAPACITY = sizeof(ComponentMask) * 8;

constexpr const ComponentID   COMPONENT_ID_NULL   = std::numeric_limits<ComponentID  >::max();
constexpr const ComponentMask COMPONENT_MASK_NULL = 0;
constexpr const ComponentSize COMPONENT_SIZE_NULL = std::numeric_limits<ComponentSize>::max();

template <typename T>
constexpr bool IsComponentType = std::is_base_of_v<Component, T>;

template <typename T>
constexpr bool IsTagType = IsComponentType<T> && std::is_empty_v<T>;

inline ComponentID getNextComponentID() {
    static ComponentID counter = 0;
    return counter++;
}

template <typename C>
inline ComponentID getComponentID() noexcept {
    static ComponentID id = getNextComponentID();
    return id;
}

template <typename C>
inline ComponentMask getComponentMask() noexcept {
    return ComponentMask(1) << getComponentID<C>();
}

template <typename... Components>
inline ComponentMask getMultiComponentMask() noexcept {
    ComponentMask mask = 0;
    ((mask |= getComponentMask<Components>()), ...);
    return mask;
}

// =============================================================================
// Component
// =============================================================================

struct Component{};

// =============================================================================
// ComponentRecord
// =============================================================================

struct ComponentRecord {
    ComponentID   id;     // unique component identifier
    ComponentMask mask;   // bitmask with single set bit at "id" (e.g. 1 << id)
    ComponentSize size;   // size in bytes of a single component element
    ComponentSize offset; // chunk array offset (used in Archetype)

    ComponentRecord()
        : id(COMPONENT_ID_NULL),
          mask(COMPONENT_MASK_NULL),
          size(COMPONENT_SIZE_NULL),
          offset(COMPONENT_SIZE_NULL) {}

    bool isTag() const noexcept { return size == 0; }
    bool isValid() const noexcept { return id != COMPONENT_ID_NULL; }
};

// =============================================================================
// ComponentManager
// =============================================================================

class ComponentManager {
public:
    ComponentManager();

    template <typename T>
    bool has() const;
    bool has(ComponentID id) const;

    template <typename T>
    bool isTag() const;
    bool isTag(ComponentID id) const;

    template <typename T>
    const ComponentRecord& getRecord() const;
    const ComponentRecord& getRecord(ComponentID id) const;

    template <typename T>
    ComponentID create();

    size_t getCount() const { return count_; }
    size_t getCapacity() const { return COMPONENT_CAPACITY; }
    bool isFull() const { return count_ > COMPONENT_CAPACITY; }

    void print() const;

private:
    std::array<ComponentRecord, COMPONENT_CAPACITY> records_;
    ComponentID count_;
};

ComponentManager::ComponentManager()
    : records_({}), count_(0) {}

template <typename T>
bool ComponentManager::has() const {
    return has(getComponentID<T>());
}

bool ComponentManager::has(ComponentID id) const {
    return id < COMPONENT_CAPACITY && records_[id].isValid();
}

template <typename T>
bool ComponentManager::isTag() const {
    return isTag(getComponentID<T>());
}

bool ComponentManager::isTag(ComponentID id) const {
    return has(id) && records_[id].isTag();
}

template <typename T>
const ComponentRecord& ComponentManager::getRecord() const {
    return getRecord(getComponentID<T>());
}

const ComponentRecord& ComponentManager::getRecord(ComponentID id) const {
    if (!has(id)) {
        throw std::runtime_error("Component does not exist");
    }
    return records_[id];
}

template <typename T>
ComponentID ComponentManager::create() {
    static_assert(IsComponentType<T>,  "Type T must inherit from ECS::Component");

    ComponentID id = getComponentID<T>();

    if (isFull()) {
        throw std::runtime_error("Component registry full");
    }

    if (id >= COMPONENT_CAPACITY) {
        throw std::runtime_error("Component ID exceeds capacity");
    }
    
    if (has(id)) {
        throw std::runtime_error(
            std::string("Component already registered: ") + typeid(T).name()
        );
    }

    ComponentRecord& r = records_[id];
    r.id   = id;
    r.mask = ComponentMask(1) << id;
    r.size = IsTagType<T> ? 0 : sizeof(T);
    count_++;

    return id;
}

void ComponentManager::print() const {
    std::cout << "components:" << std::endl;
    for (ComponentID id = 0; id < COMPONENT_CAPACITY; id++) {
        if (records_[id].isValid()) {
            const ComponentRecord& r = records_[id];
            std::cout << "  - id: "   << static_cast<int>(r.id) << std::endl;
            std::cout << "    mask: " << r.mask << std::endl;
            std::cout << "    size: " << r.size << std::endl;
        }
    }
}

} // namespace Game::ECS
#pragma once

#include "ecs/types.hpp"

#include <iostream>
#include <limits> // for std::numeric_limits
#include <type_traits>
#include <vector>

namespace Game::ECS {

// =============================================================================
// System Types
// =============================================================================

using SystemID = uint16_t;

constexpr const SystemID SYSTEM_ID_NULL = std::numeric_limits<SystemID >::max();

template <typename T>
constexpr bool IsSystemType = std::is_base_of<System, T>::value;

inline SystemID getNextSystemID() {
    static SystemID counter = 0;
    return counter++;
}

template <typename T>
inline SystemID getSystemID() {
    static SystemID id = getNextSystemID();
    return id;
}

// =============================================================================
// System
// =============================================================================

class System {
    friend class SystemManager;

public:
    virtual ~System() = default;
    virtual void run() = 0;

protected:
    ECSManager* ecs;

private:
    SystemID id_;
};

// =============================================================================
// SystemManager
// =============================================================================

class SystemManager {
public:
    ~SystemManager();

    template <typename T>
    bool has() const;
    bool has(SystemID id) const;

    template <typename T>
    System* get() const;
    System* get(SystemID id) const;

    template <typename T>
    SystemID create(ECSManager* ecs);
    template <typename T>
    void run();

    void print() const;

private:
    std::vector<System*> systems_; // TODO: make deque of system records?
};

SystemManager::~SystemManager() {
    for (System*& system : systems_) {
        delete system;
    }
}

template <typename T>
bool SystemManager::has() const {
    return has(getSystemID<T>());
}

bool SystemManager::has(SystemID id) const {
    return id < systems_.size();
}

template <typename T>
System* SystemManager::get() const {
    return get(getSystemID<T>());
}

System* SystemManager::get(SystemID id) const {
    ASSERT(has(id), "System " << id << " does not exist.");
    return systems_[id];
}

template <typename T>
SystemID SystemManager::create(ECSManager* ecs) {
    static_assert(IsSystemType<T>, "Type T is not a valid System.");
    SystemID id = getSystemID<T>();
    ASSERT(!has(id), "System already registered.");
    systems_.emplace_back(new T());
    systems_.back()->ecs = ecs;
    return id;
}

template <typename T>
void SystemManager::run() {
    static_assert(IsSystemType<T>, "Type T is not a valid System.");
    SystemID id = getSystemID<T>();
    systems_[id]->run();
}

void SystemManager::print() const {
    std::cout << "systems_:" << std::endl;
    for (const System* s : systems_) {
        std::cout << "  - id: " << s->id_ << std::endl;
    }
}


} // namespace Game::ECS
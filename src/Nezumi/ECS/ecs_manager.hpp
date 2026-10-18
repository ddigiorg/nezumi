#pragma once

#include "ecs/types.hpp"
#include "ecs/archetype.hpp"
#include "ecs/chunk.hpp"
#include "ecs/chunk_manager.hpp"
#include "ecs/component.hpp"
#include "ecs/entity.hpp"
#include "ecs/query.hpp"
#include "ecs/system.hpp"
#include "utils/assert.hpp"

// TODO: add logger
// TODO: add event OnEntityCreate and OnEntityDestroy

namespace Game::ECS {

class ECSManager {
public:
    ECSManager();
    ~ECSManager() = default;

    // === Component ===

    template <typename C>
    ComponentID registerComponent();

    template <typename C>
    bool hasComponent() const;
    bool hasComponent(ComponentID c_id) const;

    template <typename C>
    bool isTag() const;
    bool isTag(ComponentID c_id) const;

    // size_t getCompnentCount() const;

    // === Entity ===

    template<typename... Components>
    EntityID createEntity(Components&&... data);

    void removeEntity(EntityID e_id);
    bool hasEntity(EntityID e_id) const;

    size_t entityCount() const;

    // template<typename C>
    // C* getEntityComponent(EntityID e_id) const;

    // template<typename C>
    // const C* getEntityComponent(EntityID e_id) const;

    // template<typename C>
    // void setEntityComponent(EntityID e_id, const C& data);

    // template<typename C>
    // bool hasEntityComponent(EntityID e_id) const;

    // === Query ===

    template<typename... Components>
    Query query();

    // void clearQueryCache();

    // === System ===

    template <typename S>
    SystemID registerSystem();

    template <typename S>
    S* getSystem();

    template <typename S>
    void runSystem();

    template <typename S>
    bool hasSystem() const;
    bool hasSystem(SystemID s_id) const;

    // === Utility ===

    void print() const;

private:
    ArchetypeManager archetype_mgr_;
    ChunkManager     chunk_mgr_;
    ComponentManager component_mgr_;
    EntityManager    entity_mgr_;
    QueryManager     query_mgr_;
    SystemManager    system_mgr_;
};

// =============================================================================
// Constructor
// =============================================================================

ECSManager::ECSManager()
    : archetype_mgr_(component_mgr_)
    , chunk_mgr_(archetype_mgr_, entity_mgr_)
    , component_mgr_()
    , entity_mgr_()
    , query_mgr_(chunk_mgr_)
    , system_mgr_()
{}

// =============================================================================
// Component
// =============================================================================

template <typename C>
ComponentID ECSManager::registerComponent() {
    return component_mgr_.create<C>();
}

template <typename C>
bool ECSManager::hasComponent() const {
    return hasComponent(getComponentID<C>());
}

bool ECSManager::hasComponent(ComponentID c_id) const {
    return component_mgr_.has(c_id);
}

template <typename C>
bool ECSManager::isTag() const {
    return component_mgr_.isTag<C>();
}

bool ECSManager::isTag(ComponentID c_id) const {
    return component_mgr_.isTag(c_id);
}

// =============================================================================
// Entity
// =============================================================================

template<typename... Components>
EntityID ECSManager::createEntity(Components&&... data) {
    // // Validate all components are registered (compile-time + runtime checks)
    // static_assert((IsValidComponent<std::decay_t<Components>> && ...),
    //         "All types must be valid ECS components");

    // // Runtime check that components are registered
    // auto checkRegistered = [this]<typename T>() {
    //     ComponentID id = getComponentID<T>();
    //     ASSERT(component_mgr_.hasComponent(id),
    //         "Component " << typeid(T).name() << " is not registered. Call ECSManager::registerComponent<T>() first.");
    // };
    // (checkRegistered.template operator()<std::decay_t<Components>>(), ...);

    EntityID    e_id = entity_mgr_.create();
    ArchetypeID a_id = archetype_mgr_.getOrCreate<Components...>();
    chunk_mgr_.insertEntity(e_id, a_id, std::forward<Components>(data)...);

    // invalidate query cache for this archetype
    // query_mgr_.invalidateforArchetype(a_id);

    return e_id;
}

void ECSManager::removeEntity(EntityID e_id) {
    ASSERT(hasEntity(e_id), "Entity id does not exist");
    chunk_mgr_.removeEntity(e_id);
}

bool ECSManager::hasEntity(EntityID e_id) const {
    return entity_mgr_.has(e_id);
}

size_t ECSManager::entityCount() const {
    return entity_mgr_.count();
}

// =============================================================================
// Query
// =============================================================================

template<typename... Components>
Query ECSManager::query() {
    ComponentMask required_mask = getMultiComponentMask<Components...>();
    return query_mgr_.query(required_mask);
}

// =============================================================================
// System
// =============================================================================

template <typename S>
SystemID ECSManager::registerSystem() {
    return system_mgr_.create<S>(this);
}

template <typename S>
S* ECSManager::getSystem() {
    return static_cast<S*>(system_mgr_.get<S>());
}

template <typename S>
void ECSManager::runSystem() {
    system_mgr_.run<S>();
}

template <typename S>
bool ECSManager::hasSystem() const {
    return system_mgr_.has<S>();
}

bool ECSManager::hasSystem(SystemID s_id) const {
    return system_mgr_.has(s_id);
}

// =============================================================================
// Miscellaneous
// =============================================================================

void ECSManager::print() const {
    archetype_mgr_.print();
    chunk_mgr_.print();
    component_mgr_.print();
    entity_mgr_.print();
    system_mgr_.print();
}

} // namespace Game::ECS

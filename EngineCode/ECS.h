#pragma once
#include "Common/Log.h"

#include <bitset>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <set>
#include <deque>
#include <format>

#define ECS_LOGGING 0

namespace Magic
{
template <typename Tag, typename T, T default_value>
class ID {
public:
    using ValueType = T;
    static ID invalid() { return ID(); }
    ID() : m_value(default_value) {}
    explicit ID(T value) : m_value(value) {} // Prevents implict conversions from int to ID
    // explicit operator T() const { return m_value; } // Conversion operator: get back original T
    T GetValue() const { return m_value; }
    // explicit operator size_t() const { return m_value; } // Conversion operator: size_t

    friend bool operator==(ID lhs, ID rhs) { return lhs.m_value == rhs.m_value; }
    friend bool operator!=(ID lhs, ID rhs) { return lhs.m_value != rhs.m_value; }
    friend bool operator<(ID lhs, ID rhs) { return lhs.m_value < rhs.m_value; }
    friend bool operator>(ID lhs, ID rhs) { return lhs.m_value > rhs.m_value; }
    friend bool operator<=(ID lhs, ID rhs) { return lhs.m_value <= rhs.m_value; }
    friend bool operator>=(ID lhs, ID rhs) { return lhs.m_value >= rhs.m_value; }
    friend ID operator+(ID lhs, ID rhs) { return ID(lhs.m_value + rhs.m_value); }
    friend ID operator-(ID lhs, ID rhs) { return ID(lhs.m_value - rhs.m_value); }

    ID& operator+=(ID rhs) {
        m_value += rhs.m_value;
        return *this;
    }

    ID& operator-=(ID rhs) {
        m_value -= rhs.m_value;
        return *this;
    }

    // Pre-increment
    ID& operator++() {
        ++m_value;
        return *this;
    }

    // Post-increment
    ID operator++(int) {
        ID temp(*this);
        ++(*this);
        return temp;
    }

    // Pre-decrement
    ID& operator--() {
        --m_value;
        return *this;
    }

    // Post-decrement
    ID operator--(int) {
        ID temp(*this);
        --(*this);
        return temp;
    }
private:
    T m_value;
};

typedef ID<class Entity, int, -1> ENTITY_ID;
typedef ID<struct IComponent, int, -1> COMPONENT_ID;
}

// Define custom hash for ENTITY_ID
namespace std {
    template<>
    struct hash<Magic::ENTITY_ID> {
        std::size_t operator()(const Magic::ENTITY_ID& id) const noexcept {
            return hash<int>()(id.GetValue());
        }
    };
}

namespace Magic
{
// using ENTITY_ID = int;
// using COMPONENT_ID = int;

class Entity {
private:
    ENTITY_ID m_id;
public:
    Entity(ENTITY_ID id);
    inline ENTITY_ID GetId() const { return m_id; }
    void EnqueueKill() const;

    Entity(const Entity& other) = default;
    Entity& operator=(const Entity& other) = default;
    inline bool operator==(const Entity& other) const { return this->m_id == other.m_id; }
    inline bool operator!=(const Entity& other) const { return this->m_id != other.m_id; }
    inline bool operator<(const Entity& other) const { return this->m_id < other.m_id; }
    inline bool operator>(const Entity& other) const { return this->m_id > other.m_id; }

    template <typename TComponent, typename ...TArgs> void AddComponent(TArgs&& ...args);
    template <typename TComponent> void RemoveComponent();
    template <typename TComponent> bool HasComponent() const;
    template <typename TComponent> TComponent& GetComponent() const;

    // Entity's owner registry, to enable AddComponent via Entity
    class Registry* m_registry;
};

constexpr int MAX_COMPONENTS_PER_ENTITY = 32;
typedef std::bitset<MAX_COMPONENTS_PER_ENTITY> Signature;

// Interface type lets us use IComponent as a return value to a function (since Component is not a real type). For example something like IComponent& GetComponent(Entity entity); ditto for IPool vs. Pool further below.
struct IComponent {
protected:
    static COMPONENT_ID m_nextId; // This is placed in a separate base class, and not the child class(es) because we want the value to be shared across all Components
};

// Used to assign a unique id per Compontent type T
template <typename T>
struct Component : public IComponent {
    static COMPONENT_ID GetId() {
        // Initialized only once per generic type T, since each Component<T> can be considered a separate class
        // Will skip and return the same id on subsequent calls. Similar to a singleton
        static auto id = m_nextId++;
        return id;
    }
};

class System { // Operates on Entities with a certain Signature
private:
    Signature m_componentSignature;
    std::vector<Entity> m_entities;
protected:
    std::vector<Entity>& GetSystemEntities() { return m_entities; }
public:
    System() = default;
    ~System() = default;

    void AddEntity(const Entity& entity);
    [[nodiscard]] bool RemoveEntity(const Entity& entity);
    const Signature& GetComponentSignature() const { return m_componentSignature; }

    template <typename TComponent>
    void RequireComponent();
};

template <typename TComponent>
void System::RequireComponent() {
    const auto componentId = Component<TComponent>::GetId();
    m_componentSignature.set(componentId.GetValue());
}

class IPool {
public:
    virtual ~IPool() = default;
    virtual void RemoveEntityFromPool(ENTITY_ID entityId) = 0;
};

template <typename TComponent, typename TIndex>
class Pool : public IPool {
private:
    std::vector<TComponent> m_data;
    int m_packedSize;
    // packedSize is sort of a "logical size" which may be less than the "true" .size() of the vector with the idea of leaving dead components at the end
    // Notice how in Remove() we never directly set the value of m_data[indexOfLastEntity] anywhere once we move it in place of the entity whose component
    // is being removed. This is similar to likely how pop_back() works within vector itself, there's no need to clear the data and the .size() is sort of
    // a logical concept in itself. Of course our pools don't need to make any order gurantees w.r.t to the entity Ids unlike vector, which is why we build
    // on top of it with our own logical size concept.

    // TODO: std::vector<T>::size_type -> Technically use this?
    std::unordered_map<ENTITY_ID, int> m_entityIdToIndex;
    std::unordered_map<int, ENTITY_ID> m_indexToEntityId;

public:
    Pool(int defaultPoolDataCapacity = 100) : m_packedSize(0) {
        m_data.reserve(defaultPoolDataCapacity);
    }
    ~Pool() override final = default;
    bool IsEmpty() const {
        return m_packedSize == 0;
    }

    std::size_t GetSize() const {
        return m_packedSize;
    }

    void Clear() {
        m_data.clear();
        m_entityIdToIndex.clear();
        m_indexToEntityId.clear();
        m_packedSize = 0;
    }

    void Set(TIndex entityId, TComponent object) {
        if (m_entityIdToIndex.find(entityId) != m_entityIdToIndex.end()) {
            // If the component already exists for this entity, replace the data
            int entityDataIndex = m_entityIdToIndex[entityId];
            m_data[entityDataIndex] = object;
        } else {
            // Append data to the end of the data block
            int entityDataIndex = m_packedSize;
            m_entityIdToIndex[entityId] = entityDataIndex;
            m_indexToEntityId[entityDataIndex] = entityId;

            // If the packed size exceeds the "real" size of the vector, we can just push_back, let the vector handle the re-allocation strategy needed to expand the underlying capacity
            if (entityDataIndex >= static_cast<int>(m_data.size())) {
                if (m_data.size() == m_data.capacity()) { m_data.reserve(m_data.capacity() * 2); } // Explicitly control the allocation behavior, likely vector does something like this anwyays
                m_data.push_back(object);
            } else {
                m_data[entityDataIndex] = object; // Otherwise we can recycle indices
            }

            m_packedSize++;
        }
    }

    void Remove(TIndex entityId) {
        // First we move the last element to the index of the entity that is being removed
        int indexOfEntityBeingRemoved = m_entityIdToIndex[entityId];
        int indexOfLastEntity = m_packedSize - 1;

        // If it happens to be the last element, we don't need to do this
        if (indexOfEntityBeingRemoved != indexOfLastEntity) {
            m_data[indexOfEntityBeingRemoved] = m_data[indexOfLastEntity];
            // Update the last's new index
            TIndex lastEntityId = m_indexToEntityId[indexOfLastEntity];
            m_entityIdToIndex[lastEntityId] = indexOfEntityBeingRemoved;
            m_indexToEntityId[indexOfEntityBeingRemoved] = lastEntityId;
        }

        m_entityIdToIndex.erase(entityId);
        m_indexToEntityId.erase(indexOfLastEntity);
#if ECS_LOGGING 
        Logger::Info("remove entity!");
#endif
        m_packedSize--;
    }

    void RemoveEntityFromPool(ENTITY_ID entityId) override {
        if(m_entityIdToIndex.find(entityId) != m_entityIdToIndex.end()) {
            Remove(entityId);
        }
    }

    TComponent& Get(TIndex entityId) {
        int index = m_entityIdToIndex[entityId];
        return static_cast<TComponent&>(m_data[index]);
    }

    TComponent& operator[](TIndex entityId) {
        int index = m_entityIdToIndex[entityId];
        return static_cast<TComponent&>(m_data[index]);
    }

};


// Manages entities, systems, and components
class Registry {
public:
    Registry() = default;
    ~Registry()
    {
        Logger::Info(std::format("Shutting down ECS, with {} entities left", m_numEntities));
    }

    // Entity management
    [[nodiscard]] Entity EnqueueCreateEntity();
    void EnqueueKillEntity(Entity entity);

    // Component management
    template <typename TComponent, typename ...TArgs> void AddComponent(Entity entity, TArgs&& ...args);
    template <typename TComponent> void RemoveComponent(Entity entity);
    template <typename TComponent> bool HasComponent(Entity entity) const;
    template <typename TComponent> TComponent& GetComponent(Entity entity) const;

    // System management
    template <typename TSystem, typename ...TArgs> void AddSystem(TArgs&& ...args);
    template <typename TSystem> void RemoveSystem();
    template <typename TSystem> bool HasSystem() const;
    template <typename TSystem> TSystem& GetSystem() const;

    // Add/remove an entity to/from the appropriate systems based on its component signature
    void AddEntityToSystems(Entity entity);
    void RemoveEntityFromSystems(Entity entity);

    void Update(); // Runs at the end of the main loop Update();

private:
    int m_numEntities = 0;

    // List of free entity ids that were previously killed
    std::deque<ENTITY_ID> m_freeIds;

    // Temp awaiting creation/destruction in the next Registry Update()
    std::set<Entity> m_entitiesToBeAdded;
    std::set<Entity> m_entitiesToBeKilled;

    // Each pool contains all the data for a certain component type
    //
    // vector index is a component id
    // pool index of the pool is an entity id
    std::vector<std::shared_ptr<IPool>> m_componentPools;

    // Lets us know which components are turned "on" for a given entity
    //
    // vector index is an entity id
    std::vector<Signature> m_entityComponentSignatures;

    std::unordered_map<std::type_index, std::shared_ptr<System>> m_systems;
};

template <typename TComponent, typename ...TArgs>
void Registry::AddComponent(Entity entity, TArgs&& ...args) {
    const COMPONENT_ID componentId = Component<TComponent>::GetId();
    const ENTITY_ID entityId = entity.GetId();

    // Ids may be assigned (via RequireComponent) in a different order than concrete components are created
    if (componentId >= COMPONENT_ID(static_cast<COMPONENT_ID::ValueType>(m_componentPools.size()))) {
        m_componentPools.resize(componentId.GetValue() + 1, nullptr); // Usually this is a constant time operation unless capacity < new size, ditto for each pool below
    }

    // If a pool doesn't exist for this component yet (Components may not be added/accessed in order)
    if (!m_componentPools[componentId.GetValue()]) {
        m_componentPools[componentId.GetValue()] = std::make_shared<Pool<TComponent, ENTITY_ID>>();
    }

    // Get the component pool for the component type
    // Need to explicitly downcast from IPool to Pool ptr
    std::shared_ptr<Pool<TComponent, ENTITY_ID>> componentPool = std::static_pointer_cast<Pool<TComponent, ENTITY_ID>>(m_componentPools[componentId.GetValue()]);

    // Create a new compontent of type T from the passed in args
    TComponent newComponent(std::forward<TArgs>(args)...);

    // Add to the component pool list for that entity
    componentPool->Set(entityId, newComponent);

    // Update entity component signature
    m_entityComponentSignatures[entityId.GetValue()].set(componentId.GetValue());
#if ECS_LOGGING 
    Logger::Info(std::format("Component id: {} was added to entity id: {}", componentId.GetValue(), entityId.GetValue()));
    Logger::Info(std::format("Component id: {} --> pool size: {}", componentId.GetValue(), componentPool->GetSize()));
#endif
}

template <typename TComponent>
void Registry::RemoveComponent(Entity entity) {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();


    // Remove component from the component list of that entity
    std::shared_ptr<Pool<TComponent, ENTITY_ID>> componentPool = std::static_pointer_cast<Pool<TComponent, ENTITY_ID>>(m_componentPools[componentId.GetValue()]);
    componentPool->Remove(entityId);

    m_entityComponentSignatures[entityId.GetValue()].set(componentId.GetValue(), false);
#if ECS_LOGGING 
    Logger::Info(std::format("Component id: {} was removed from entity id: {}", componentId.GetValue(), entityId.GetValue()));
#endif
}

template <typename TComponent>
bool Registry::HasComponent(Entity entity) const {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();
    return m_entityComponentSignatures[entityId.GetValue()].test(componentId.GetValue());
}

template <typename TComponent>
TComponent& Registry::GetComponent(Entity entity) const {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();
    // TODO: Check if component actually exists
    auto componentPool = std::static_pointer_cast<Pool<TComponent, ENTITY_ID>>(m_componentPools[componentId.GetValue()]);
    return componentPool->Get(entityId);
}

template <typename TComponent, typename ...TArgs>
void Entity::AddComponent(TArgs&& ...args) {
    m_registry->AddComponent<TComponent>(*this, std::forward<TArgs>(args)...);
}

template <typename TComponent>
void Entity::RemoveComponent() {
    m_registry->RemoveComponent<TComponent>(*this);
}

template <typename TComponent>
bool Entity::HasComponent() const {
    return m_registry->HasComponent<TComponent>(*this);
}

template <typename TComponent>
TComponent& Entity::GetComponent() const {
    return m_registry->GetComponent<TComponent>(*this);
}

template <typename TSystem, typename... TArgs>
void Registry::AddSystem(TArgs &&...args) {
    m_systems.insert(std::make_pair(
        std::type_index(typeid(TSystem))
        , std::make_shared<TSystem>(std::forward<TArgs>(args)...)
    ));
}

template <typename TSystem>
void Registry::RemoveSystem() {
    if (m_systems.find(std::type_index(typeid(TSystem))) != m_systems.end()) {
        m_systems.erase(std::type_index(typeid(TSystem)));
    }
}

template <typename TSystem>
bool Registry::HasSystem() const {
    return (m_systems.find(std::type_index(typeid(TSystem))) != m_systems.end());
}

template <typename TSystem>
TSystem& Registry::GetSystem() const {
    auto system = m_systems.find(std::type_index(typeid(TSystem)));
    return *(std::static_pointer_cast<TSystem>(system->second)); // TODO: Assumes system exists
}



}
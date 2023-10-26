#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>
#include "component_pool.hpp"
#include "gameobject.hpp"
#include <vector>
#include "../graphics/engine.hpp"
#include "transform_component.cpp"
#include "../physics/spatial_acceleration_structure.hpp"
#include "pointlight_component.hpp"
#include <optional>

namespace ComponentRegistry {
    enum ComponentBitIndex {
        TransformComponentBitIndex = 0,
        RenderComponentBitIndex = 1,
        ColliderComponentBitIndex = 2,
        RigidbodyComponentBitIndex = 3,
        PointlightComponentBitIndex = 4
    };
}

struct CreateGameObjectParams {
    unsigned int meshId;
    unsigned int textureId;
    unsigned int shaderId;
    
    std::vector<ComponentRegistry::ComponentBitIndex> requestedComponents;

    CreateGameObjectParams(const std::vector<ComponentRegistry::ComponentBitIndex> componentList):
    meshId(0),
    textureId(0),
    shaderId(0),
    requestedComponents(componentList)
    {}
};

// Just a little pointer wrapper for gameobjects that throws an error when trying to dereference a nullptr to a component (gameobjects have a nullptr to components they don't have)
// Still the gameobject's job to get and return objects to/from the pool since the pool used depends on the exact set of components the gameobject uses.
template<typename T>
class ComponentHandle {
    public:
    ComponentHandle(T* const comp_ptr);
    T& operator*() const;
    T* operator->() const;
    T* const ptr;
};

// The gameobject system uses ECS (google it).
class GameObject {
    public:
    std::string name; // just an identifier i have mainly for debug reasons, scripts could also use it i guess

    // TODO: any way to avoid not storing ptrs for components we don't have?
    ComponentHandle<TransformComponent> transformComponent;
    ComponentHandle<GraphicsEngine::RenderComponent> renderComponent;
    ComponentHandle<SpatialAccelerationStructure::ColliderComponent> colliderComponent;

    ~GameObject();

    private:
    // no copy constructing gameobjects.
    GameObject(const GameObject&) = delete; 

    // private because ComponentRegistry makes it and also because it needs to return a shared_ptr.
    GameObject(const CreateGameObjectParams& params);
    
};

// This file stores all components in a way that keeps them both organized and cache-friendly.
// Basically entites with the same set of components have their components stored together.
// Pointers to components are never invalidated thanks to component pool fyi.
namespace ComponentRegistry {
    // How many different component classes there are. 
    static inline const unsigned int N_COMPONENT_TYPES = 8; 

    // Given a vector of BitIndexes for components a system wants to iterate over, returns a vector of vectors of void pointers to component pools, where each interior vector contains a component pool for each of the requested components, in the order they were requested.
    // Yeah its not type safe, TODO getting rid of the void* would be nice
    // For example, if you called this with {TransformComponentBitIndex, RenderComponentBitIndex}, you'd get {{ComponentPool<TransformComponent>*, ComponentPool<RenderComponent>*}, {ComponentPool<TransformComponent>*, ComponentPool<RenderComponent>*}, ...}
    std::vector<std::vector<void*>> GetSystemComponents(std::vector<ComponentBitIndex> requestedComponents);

    // Returns a new game object with the given components.
    std::shared_ptr<GameObject> NewGameObject(std::vector<ComponentBitIndex> requestedComponents);

    // Call at end of program. Deletes all gameobjects, components, and component pools.
    void CleanupComponents();
};
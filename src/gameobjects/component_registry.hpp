#pragma once
#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>
#include "component_pool.hpp"
#include "component_registry.hpp"
#include <vector>
#include "../graphics/engine.hpp"
#include "transform_component.cpp"
#include "../physics/spatial_acceleration_structure.hpp"
#include "pointlight_component.hpp"
#include <optional>

struct CreateGameObjectParams;

namespace ComponentRegistry {
    enum ComponentBitIndex {
        TransformComponentBitIndex = 0,
        RenderComponentBitIndex = 1,
        ColliderComponentBitIndex = 2,
        RigidbodyComponentBitIndex = 3,
        PointlightComponentBitIndex = 4
    };

    // How many different component classes there are. 
    static inline const unsigned int N_COMPONENT_TYPES = 8; 

    // Returns a new game object with the given components.
    std::shared_ptr<GameObject> NewGameObject(const CreateGameObjectParams& params);
}

struct CreateGameObjectParams {
    unsigned int meshId;
    unsigned int textureId;
    unsigned int shaderId;

    CreateGameObjectParams(const std::vector<ComponentRegistry::ComponentBitIndex> componentList):
    meshId(0),
    textureId(0),
    shaderId(0)
    {
        // bitset defaults to all false so we good
        for (auto & i : componentList) {
            requestedComponents[i] = true;
        }
    }

    private:
    friend std::shared_ptr<GameObject> ComponentRegistry::NewGameObject(const CreateGameObjectParams& params);
    std::bitset<ComponentRegistry::N_COMPONENT_TYPES> requestedComponents;
};

// Just a little pointer wrapper for gameobjects that throws an error when trying to dereference a nullptr to a component (gameobjects have a nullptr to any components they don't have)
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
    friend class std::shared_ptr<GameObject>;
    GameObject(const CreateGameObjectParams& params, std::vector<void*> components); // components is not type safe because component registry weird, index is ComponentBitIndex, value is nullptr or ptr to componeny
    
};

// This file stores all components in a way that keeps them both organized and cache-friendly.
// Basically entites with the same set of components have their components stored together.
// Pointers to components are never invalidated thanks to component pool fyi.
namespace ComponentRegistry {
    // Given a vector of BitIndexes for components a system wants to iterate over, returns a vector of vectors of void pointers to component pools, where each interior vector contains a component pool for each of the requested components, in the order they were requested.
    // Yeah its not type safe, TODO getting rid of the void* would be nice
    // For example, if you called this with {TransformComponentBitIndex, RenderComponentBitIndex}, you'd get {{ComponentPool<TransformComponent>*, ComponentPool<RenderComponent>*}, {ComponentPool<TransformComponent>*, ComponentPool<RenderComponent>*}, ...}
    std::vector<std::vector<void*>> GetSystemComponents(std::vector<ComponentBitIndex> requestedComponents);
    
    std::shared_ptr<GameObject> NewGameObject(const CreateGameObjectParams& params);
    

    // Call at end of program. Deletes all gameobjects, components, and component pools.
    void CleanupComponents();
};
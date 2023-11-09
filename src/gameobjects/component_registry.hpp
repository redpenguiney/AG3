#pragma once
#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <memory>
#include <tuple>
#include <vector>
#include "component_pool.hpp"
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
    unsigned int physMeshId; // 0 if you want automatically generated
    unsigned int meshId;
    unsigned int textureId;
    unsigned int shaderId;

    CreateGameObjectParams(const std::vector<ComponentRegistry::ComponentBitIndex> componentList):
    physMeshId(0),
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

// templates can't go in cpps
template<typename T>
ComponentHandle<T>::ComponentHandle(T* const comp_ptr) : ptr(comp_ptr) {}

template<typename T>
T& ComponentHandle<T>::operator*() const {assert(ptr != nullptr); return *ptr;}

template<typename T>
T* ComponentHandle<T>::operator->() const {assert(ptr != nullptr); return ptr;};

class RigidbodyComponent;

// The gameobject system uses ECS (google it).
class GameObject {
    public:
    std::string name; // just an identifier i have mainly for debug reasons, scripts could also use it i guess

    // TODO: any way to avoid not storing ptrs for components we don't have?
    ComponentHandle<TransformComponent> transformComponent;
    ComponentHandle<GraphicsEngine::RenderComponent> renderComponent;
    ComponentHandle<RigidbodyComponent> rigidbodyComponent;
    ComponentHandle<SpatialAccelerationStructure::ColliderComponent> colliderComponent;

    ~GameObject();

    protected:
    
    // no copy constructing gameobjects.
    GameObject(const GameObject&) = delete; 

    // private because ComponentRegistry makes it and also because it needs to return a shared_ptr.
    friend class std::shared_ptr<GameObject>;
    GameObject(const CreateGameObjectParams& params, std::array<void*, ComponentRegistry::N_COMPONENT_TYPES> components); // components is not type safe because component registry weird, index is ComponentBitIndex, value is nullptr or ptr to componeny
    
};

// This file stores all components in a way that keeps them both organized and cache-friendly.
// Basically entites with the same set of components have their components stored together.
// Pointers to components are never invalidated thanks to component pool fyi.
namespace ComponentRegistry {
    // To hide all the non-type safe stuff, we need an iterator that just lets people iterate through the components of all gameobjects that have certain components (i.e give me all pairs of transform + render)
    template <typename ... Args>
    class Iterator {
        public: 

        // std libraries expect iterators to do this
        using iterator_category = std::forward_iterator_tag; // this is a forward iterator
        using difference_type   = std::ptrdiff_t;
        using value_type        = std::tuple<Args...>;
        using pointer           = value_type*;  // or also value_type*
        using reference         = value_type&;  // or also value_type&

        // forward iterator stuff
        Iterator<Args...>& operator++(); // prefix
        reference operator*();
        pointer operator->();
        friend bool operator==(const Iterator<Args...>& a, const Iterator<Args...>& b);
        friend bool operator!=(const Iterator<Args...>& a, const Iterator<Args...>& b);
        Iterator<Args...> operator++(int); //postfix

        Iterator<Args...> begin();
        Iterator<Args...> end();

        Iterator(std::vector<value_type> pools);

        private:
        std::vector<value_type> pools;
        unsigned int poolIndex;
        unsigned int componentIndex;
    };

    inline std::unordered_map<GameObject*, std::shared_ptr<GameObject>> GAMEOBJECTS;

    // Given a vector of BitIndexes for components a system wants to iterate over, returns a vector of vectors of void pointers to component pools, where each interior vector contains a void* to component pool for each component, in the order given by the enum, or nullptr if not wanted.
    // Yeah its not type safe, TODO getting rid of the void* would be nice
    // For example, if you called this with {TransformComponentBitIndex, RenderComponentBitIndex}, you'd get {{ComponentPool<TransformComponent>*, ComponentPool<RenderComponent>*}, {ComponentPool<TransformComponent>*, ComponentPool<RenderComponent>*}, ...}
    template <typename ... Args>
    Iterator<Args...> GetSystemComponents();
    
    std::shared_ptr<GameObject> NewGameObject(const CreateGameObjectParams& params);
    

    // Call before destroying graphics engine, SAS, etc. at end of program, as gameobject destructors need to access those things.
    // Won't work if there are shared_ptr<GameObject>s outside the GAMEOBJECTS map (inside lua code), so make sure all lua code is eliminated before calling.
    void CleanupComponents();
};


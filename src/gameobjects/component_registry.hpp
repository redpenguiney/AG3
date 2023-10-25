#pragma once
#include <array>
#include <memory>
#include <vector>
#include "component_pool.hpp"
#include "gameobject.hpp"

class GameObject;
inline std::unordered_map<GameObject*, std::shared_ptr<GameObject>> GAMEOBJECTS;

struct CreateGameObjectParams {
    bool haveGraphics;
    unsigned int meshId;
    unsigned int textureId;
    unsigned int shaderId;
    
    bool haveCollisions;
    bool havePhysics;

    bool havePointLight;

    CreateGameObjectParams():
    haveGraphics(false),
    meshId(0),
    textureId(0),
    shaderId(0),

    haveCollisions(false),
    havePhysics(false),

    havePointLight(false) {}
};

// The gameobject system uses ECS (google it).
class GameObject {
    public:
    std::string name; // just an identifier i have mainly for debug reasons, scripts could also use it i guess

    // fyi the const in this position means that the address the pointer points to won't change, not that the pointer points to constant data
    // nullptr if gameobject doesn't have that component

    GraphicsEngine::RenderComponent* const renderComponent;
    TransformComponent* const transformComponent;
    SpatialAccelerationStructure::ColliderComponent* const colliderComponent;

    PointLightComponent* const pointLightComponent;

    static void Cleanup();

    ~GameObject();

    private:
        // todo: seems mid that this exists
        bool deleted;

        // no copy constructing gameobjects.
        GameObject(const GameObject&) = delete; 

        // private because ComponentRegistry makes it and also because it needs to return a shared_ptr.
        GameObject(const CreateGameObjectParams& params);
        
};

// This file stores all components in a way that keeps them both organized and cache-friendly.
// Pointers to components remain valid thanks to component pool fyi.
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

    // Given a vector of BitIndexes for components a system wants to iterate over, returns a vector of vectors of void pointers to component pools, where each interior vector contains a component pool for each of the requested components, in the order they were requested.
    // Yeah its not type safe, TODO getting rid of the void* would be nice
    // For example, if you called this with {TransformComponentBitIndex, RenderComponentBitIndex}, you'd get {{ComponentPool<TransformComponent>*, ComponentPool<RenderComponent>*}, {ComponentPool<TransformComponent>*, ComponentPool<RenderComponent>*}, ...}
    std::vector<std::vector<void*>> GetSystemComponents(std::vector<ComponentBitIndex> requestedComponents);

    // Returns a new game object with the given components.
    std::shared_ptr<GameObject> NewGameObject(std::vector<ComponentBitIndex> requestedComponents);
};
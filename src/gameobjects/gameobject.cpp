#pragma once
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <memory>
#include<unordered_set>
#include <vector>
#include "../graphics/engine.cpp"
#include "transform_component.cpp"
#include "../physics/spatial_acceleration_structure.cpp"

class GameObject;
std::unordered_map<GameObject*, std::shared_ptr<GameObject>> GAMEOBJECTS;

// TODO
struct CreateGameobjectParams;

// The gameobject system uses ECS (google it).
class GameObject {
    public:

    // all gameobjects will reserve a render, transform, and collision component even if they don't need them to ensure cache stuff
        // TODO: this might not be neccesary
    // GraphicsEngine and SpatialAccelerationStructure rely on this behavior, DO NOT mess with it
    // fyi the const in this position means that the address the pointer points to won't change, not that the pointer points to constant data
    GraphicsEngine::RenderComponent* const renderComponent;
    TransformComponent* const transformComponent;
    SpatialAccelerationStructure::ColliderComponent* const colliderComponent;

    static std::shared_ptr<GameObject> New(unsigned int meshId, unsigned int textureId, bool haveCollisions = true, bool havePhysics = false) {
        auto rawPtr = new GameObject(meshId, textureId, haveCollisions, havePhysics);
        auto ptr = std::shared_ptr<GameObject>(rawPtr);
        GAMEOBJECTS.emplace(rawPtr, ptr);
        return ptr;
    }    

    // NOTE: only removes shared_ptr from GAMEOBJECTS, destructor will not be called until all other shared_ptrs to this gameobject are deleted.
        // Those shared_ptrs remain completely valid and can be read/written freely (although why would you if you're destroying it???).
    // TODO: when Destroy() is called, should still make it stop being drawn and stop physics.
    void Destroy() {
        assert(!deleted);
        deleted = true;
        GAMEOBJECTS.erase(this);
    }

    // Call at end of program.
    // TODO: unclear why this exists or needs to be called
    // Simply calls Destroy() on all gameobjects that have not been destroyed yet.
    static void Cleanup() {
        std::vector<std::shared_ptr<GameObject>> addresses; // can't remove them inside the map iteration because that would mess up the map iterator
        for (auto & [key, gameobject] : GAMEOBJECTS) {
            (void)key;
            addresses.push_back(gameobject);
        }
        for (auto & gameobject: addresses) {
            gameobject->Destroy();
        }
    }

    ~GameObject() {
        renderComponent->Destroy();
        transformComponent->Destroy();
        GAMEOBJECTS.erase(this); 
    };

    private:
        bool deleted;

        // no copy constructing gameobjects.
        GameObject(const GameObject&) = delete; 

        GameObject(unsigned int meshId, unsigned int textureId, bool haveCollider, bool havePhysics):
        renderComponent(GraphicsEngine::RenderComponent::New(meshId, textureId)),
        transformComponent(TransformComponent::New()),
        colliderComponent(SpatialAccelerationStructure::ColliderComponent::New(transformComponent))
        {
            deleted = false;
            colliderComponent->live = false;
        };
        
};
#pragma once
#include <cassert>
#include <cstdio>
#include <memory>
#include<unordered_set>
#include <vector>
#include "../graphics/engine.cpp"
#include "transform_component.cpp"

class GameObject;
std::unordered_map<GameObject*, std::shared_ptr<GameObject>> GAMEOBJECTS;

// The gameobject system uses ECS (google it).
class GameObject {
    public:

    // all gameobjects will reserve a render, transform, collision, and physics component even if they don't need them to ensure cache stuff
        // TODO: this might not be neccesary
    // GraphicsEngine and PhysicsEngine rely on this behavior, DO NOT mess with it
    GraphicsEngine::RenderComponent* const renderComponent;
    TransformComponent* const transformComponent;

    static std::shared_ptr<GameObject> New(unsigned int meshId, unsigned int textureId, bool collides = true) {
        auto rawPtr = new GameObject(meshId, textureId);
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

    // Call at end of program before cleaning up singletons (like GraphicsEngine).
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

        GameObject(unsigned int meshId, unsigned int textureId):
        renderComponent(GraphicsEngine::RenderComponent::New(meshId, textureId)),
        transformComponent(TransformComponent::New()) 
        {
            deleted = false;
        };
        
};
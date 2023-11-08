#include "component_registry.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <vector>
#include "rigidbody_component.hpp"

// some wacky function i copypasted from https://stackoverflow.com/questions/8147027/how-do-i-call-stdmake-shared-on-a-class-with-only-protected-or-private-const .
// Lets me put use std::make_shared on stuff with private constructors
template < typename Object, typename... Args >
inline std::shared_ptr< Object >
protected_make_shared( Args&&... args )
{
  struct helper : public Object
  {
    helper( Args&&... args )
      : Object{ std::forward< Args >( args )... }
    {}
  };

  return std::make_shared< helper >( std::forward< Args >( args )... );
}

namespace ComponentRegistry {
    
    // Stores all the component pools.
    // Bitset has a bit for each component class, if its 1 then the value corresonding to that key stores gameobjects with that component. (but only if the gameobject stores all the exact same components as the bitset describes)
    std::unordered_map<std::bitset<N_COMPONENT_TYPES>, std::array<void*, N_COMPONENT_TYPES>> componentBuckets;

    // TODO: might be good to optimize by just precalculating this and updating when new gameobject component combination is added
    std::vector<std::array<void*, N_COMPONENT_TYPES>> GetSystemComponents(std::vector<ComponentBitIndex> requestedComponents) {

        std::vector<std::array<void*, N_COMPONENT_TYPES>> poolsToReturn;

        for (auto & [bitset, pools] : componentBuckets) {
            //std::cout << "Considering bucket with bitset " << bitset << " to supply "; for (auto & i: requestedComponents) {std::cout << i << " ";} std:: cout << ".\n";
            unsigned int i = 0;
            for (auto & bitIndex: requestedComponents) {
                if (bitset[bitIndex] == false) {
                    //std::cout << "Rejected bucket, missing index " << bitIndex << ".\n";
                    // this bucket is missing a pool for one of the requested components, don't send it to the system
                    goto innerLoopEnd;
                }
                i++;
            }
            // this pool stores gameobjects with all the components we want, return it
            poolsToReturn.push_back(pools);

            innerLoopEnd:;
        }

        return poolsToReturn;
    }

    std::shared_ptr<GameObject> NewGameObject(const CreateGameObjectParams& params) {
        // make sure there are the needed component pools for this kind of gameobject
        if (!componentBuckets.count(params.requestedComponents)) {
            componentBuckets[params.requestedComponents] = std::array<void*, N_COMPONENT_TYPES> {{
                [TransformComponentBitIndex] = params.requestedComponents[TransformComponentBitIndex] ? new ComponentPool<TransformComponent>() : nullptr,
                [RenderComponentBitIndex] = params.requestedComponents[RenderComponentBitIndex] ? new ComponentPool<GraphicsEngine::RenderComponent>() : nullptr,
                [ColliderComponentBitIndex] = params.requestedComponents[ColliderComponentBitIndex] ? new ComponentPool<SpatialAccelerationStructure::ColliderComponent>() : nullptr,
                [RigidbodyComponentBitIndex] = params.requestedComponents[RigidbodyComponentBitIndex] ? new ComponentPool<RigidbodyComponent>() : nullptr
            }};
            //std::cout << "RENDER COMP POOL PAGE AT " << ((ComponentPool<GraphicsEngine::RenderComponent>*)(componentBuckets[params.requestedComponents][RenderComponentBitIndex]))->pools[0] << "\n";
        }

        // Get components for the gameobject
        std::array<void*, N_COMPONENT_TYPES> components;
        for (unsigned int i = 0; i < params.requestedComponents.size(); i++) {
            if (params.requestedComponents[i] == true) { // if the gameobject wants this component
                switch (i) {
                case TransformComponentBitIndex:
                components[i] = ((ComponentPool<TransformComponent>*)(componentBuckets.at(params.requestedComponents).at(i)))->GetNew();
                break;
                case RenderComponentBitIndex:
                components[i] = ((ComponentPool<GraphicsEngine::RenderComponent>*)(componentBuckets.at(params.requestedComponents).at(i)))->GetNew();
                break;
                case ColliderComponentBitIndex:
                components[i] = ((ComponentPool<SpatialAccelerationStructure::ColliderComponent>*)(componentBuckets.at(params.requestedComponents).at(i)))->GetNew();
                break;
                case RigidbodyComponentBitIndex:
                components[i] = ((ComponentPool<RigidbodyComponent>*)(componentBuckets.at(params.requestedComponents).at(i)))->GetNew();
                break;
                default:
                std::printf("you goofy goober you didn't make a case here for index %u\n", i);
                abort();
                break;
                }
            }
            else {
                components[i] = nullptr;
            }
        }

        auto ptr = protected_make_shared<GameObject>(params, components);
        GAMEOBJECTS[ptr.get()] = ptr;
        return ptr;
    }

    void CleanupComponents() {
        GAMEOBJECTS.clear(); // no way its this simple
    }
};

GameObject::~GameObject() {

    //std::cout << "Destroying.\n";
    // ComponentRegistry::gameObjects.erase(this); The only way the destructor could have been called if someone already erased that.
    if (transformComponent.ptr) {transformComponent->Destroy();}
    if (renderComponent.ptr) {renderComponent->Destroy();}
    if (colliderComponent.ptr) {colliderComponent->Destroy();}
    if (rigidbodyComponent.ptr) {rigidbodyComponent->Destroy();}
    // if (pointLightComponent.ptr) {pointLightComponent->Destroy();}
    //std::cout << "Returning to pool.\n";
    if (transformComponent.ptr) {transformComponent->pool->ReturnObject(transformComponent.ptr);}
    if (renderComponent.ptr) {renderComponent->pool->ReturnObject(renderComponent.ptr);}
    if (colliderComponent.ptr) {colliderComponent->pool->ReturnObject(colliderComponent.ptr);}
    if (rigidbodyComponent.ptr) {rigidbodyComponent->pool->ReturnObject(rigidbodyComponent.ptr);}
};

GameObject::GameObject(const CreateGameObjectParams& params, std::array<void*, ComponentRegistry::N_COMPONENT_TYPES> components):
    // a way to make this less verbose and more type safe would be nice
    transformComponent((TransformComponent*)components[ComponentRegistry::TransformComponentBitIndex]),
    renderComponent((GraphicsEngine::RenderComponent*)components[ComponentRegistry::RenderComponentBitIndex]),  
    colliderComponent((SpatialAccelerationStructure::ColliderComponent*)components[ComponentRegistry::ColliderComponentBitIndex]),
    rigidbodyComponent((RigidbodyComponent*)components[ComponentRegistry::RigidbodyComponentBitIndex])
{
    assert(transformComponent.ptr != nullptr);
    transformComponent->Init();
    if (renderComponent.ptr) {renderComponent->Init(params.meshId, params.textureId, params.shaderId != 0 ? params.shaderId: GraphicsEngine::Get().GetDefaultShaderId());}
    if (colliderComponent.ptr) {
        std::shared_ptr<PhysicsMesh> physMesh;
        if (params.physMeshId == 0) {
            if (params.meshId == 0) {
                std::cout << "ERROR: When trying to create a ColliderComponent, no PhysicsMeshId was given, and no MeshId was given to produce one with.\n";
                abort();
            }
            physMesh = PhysicsMesh::New(Mesh::Get(params.meshId));
        }
        else {
            physMesh = PhysicsMesh::Get(params.physMeshId);
        }
        colliderComponent->Init(this, physMesh);
    };
    if (rigidbodyComponent.ptr) {rigidbodyComponent->Init();}
    name = "GameObject";
};
#include "component_registry.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <vector>

namespace ComponentRegistry {
    
    // Stores all the component pools.
    // Bitset has a bit for each component class, if its 1 then the value corresonding to that key stores gameobjects with that component. (but only if the gameobject stores all the exact same components as the bitset describes)
    std::unordered_map<std::bitset<N_COMPONENT_TYPES>, std::array<void*, N_COMPONENT_TYPES>> componentBuckets;

    std::unordered_map<GameObject*, std::shared_ptr<GameObject>> gameObjects;

    // Notice that gameObjects is declared after componentBuckets, which means gameObjects will be destructed before componentBuckets, which means we're good.

    std::vector<std::vector<void*>> GetSystemComponents(std::vector<ComponentBitIndex> requestedComponents) {

        std::vector<std::vector<void*>> poolsToReturn;

        for (auto & [bitset, pools] : componentBuckets) {
            std::vector<void*> matchingPools;
            for (auto & bitIndex: requestedComponents) {
                if (bitset[bitIndex] == false) {
                    // this bucket is missing a pool for one of the requested components, don't send it to the system
                    goto innerLoopEnd;
                }
                else {
                    matchingPools.push_back(pools.at(bitIndex));
                }
            }
            // this pool stores gameobjects with all the components we want, return it
            poolsToReturn.push_back(matchingPools);

            innerLoopEnd:;
        }

        return poolsToReturn;
    }

    std::shared_ptr<GameObject> NewGameObject(const CreateGameObjectParams& params) {
        // make sure there are the needed component pools for this kind of gameobject
        if (!componentBuckets.count(params.requestedComponents)) {
            componentBuckets[params.requestedComponents] = std::array<void*, N_COMPONENT_TYPES> {
                params.requestedComponents[TransformComponentBitIndex] ? new ComponentPool<TransformComponent>() : nullptr,
                params.requestedComponents[RenderComponentBitIndex] ? new ComponentPool<GraphicsEngine::RenderComponent>() : nullptr,
                params.requestedComponents[ColliderComponentBitIndex] ? new ComponentPool<SpatialAccelerationStructure::ColliderComponent>() : nullptr,
                //params.requestedComponents[RigidBodyBitIndex] ? new ComponentPool<TransformComponent>() : nullptr
            };
        }

        // Get components for the gameobject
        std::vector<void*> components;
        for (int i = 0; i < params.requestedComponents.size(); i++) {
            if (params.requestedComponents[i] == true) { // if the gameobject wants this component
                switch (i) {
                case TransformComponentBitIndex:
                components[i] = (ComponentPool<TransformComponent>*)(componentBuckets[params.requestedComponents][i]);
                break;
                case RenderComponentBitIndex:
                components[i] = (ComponentPool<GraphicsEngine::RenderComponent>*)(componentBuckets[params.requestedComponents][i]);
                break;
                case ColliderComponentBitIndex:
                components[i] = (ComponentPool<SpatialAccelerationStructure::ColliderComponent>*)(componentBuckets[params.requestedComponents][i]);
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

        auto ptr = std::make_shared<GameObject>(params, components);
        gameObjects[ptr.get()] = ptr;
        return ptr;
    }
};

template<typename T>
ComponentHandle<T>::ComponentHandle(T* const comp_ptr) : ptr(comp_ptr) {}

template<typename T>
T& ComponentHandle<T>::operator*() const {assert(ptr != nullptr); return *ptr;}

template<typename T>
T* ComponentHandle<T>::operator->() const {assert(ptr != nullptr); return ptr;};

GameObject::~GameObject() {
    ComponentRegistry::gameObjects.erase(this); 
};

GameObject::GameObject(const CreateGameObjectParams& params, std::vector<void*> components):
    // a way to make this less verbose and more type safe would be nice
    renderComponent((GraphicsEngine::RenderComponent*)components[ComponentRegistry::RenderComponentBitIndex]),
    transformComponent((TransformComponent*)components[ComponentRegistry::TransformComponentBitIndex]),
    colliderComponent((SpatialAccelerationStructure::ColliderComponent*)components[ComponentRegistry::ColliderComponentBitIndex])

{
    assert(transformComponent.ptr != nullptr);
    transformComponent->Init();
    if (renderComponent.ptr) {renderComponent->Init();}
    name = "GameObject";
};
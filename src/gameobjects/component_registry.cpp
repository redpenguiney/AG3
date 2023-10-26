#include "component_registry.hpp"
#include <algorithm>
#include <array>
#include <memory>
#include <unordered_map>
#include <bitset>
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
        return std::make_shared<GameObject>(params);
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

GameObject::GameObject(const CreateGameObjectParams& params):
    renderComponent((std::find(params.requestedComponents.begin(), params.requestedComponents.end(), ComponentRegistry::RenderComponentBitIndex) != params.requestedComponents.end()) ? )
{
    name = "GameObject";
};
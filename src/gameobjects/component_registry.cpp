#include "component_registry.hpp"
#include <array>
#include <unordered_map>
#include <bitset>
#include <vector>

namespace ComponentRegistry {
    // Stores all the component pools.
    // Bitset has a bit for each component class, if its 1 then the value corresonding to that key stores gameobjects with that component. (but only if the gameobject stores all the exact same components as the bitset describes)
    std::unordered_map<std::bitset<N_COMPONENT_TYPES>, std::array<void*, N_COMPONENT_TYPES>> ComponentBuckets;

    std::vector<std::vector<void*>> GetSystemComponents(std::vector<ComponentBitIndex> requestedComponents) {

        std::vector<std::vector<void*>> poolsToReturn;

        for (auto & [bitset, pools] : ComponentBuckets) {
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
            poolsToReturn.push_back(matchingPools);

            innerLoopEnd:;
        }
    }


};

GameObject::~GameObject() {
    GAMEOBJECTS.erase(this); 
};

void GameObject::Cleanup() {
    std::vector<std::shared_ptr<GameObject>> addresses; // can't remove them inside the map iteration because that would mess up the map iterator
    for (auto & [key, gameobject] : GAMEOBJECTS) {
        (void)key;
        addresses.push_back(gameobject);
    }
    for (auto & gameobject: addresses) {
        gameobject->colliderComponent->gameobject = nullptr;
        gameobject->Destroy();
    }
}

GameObject::GameObject(const CreateGameObjectParams& params):
    renderComponent(GraphicsEngine::RenderComponent::New(params.meshId, params.textureId, params.haveGraphics)),
    transformComponent(TransformComponent::New()),
    colliderComponent(SpatialAccelerationStructure::ColliderComponent::New(nullptr)), // it needs a shared_ptr so we need to set that in New()
    pointLightComponent((params.havePointLight) ? new PointLightComponent(transformComponent): nullptr)
{
    name = "GameObject";
    deleted = false;
    colliderComponent->live = params.haveCollisions;
    if (params.haveCollisions) {
        SpatialAccelerationStructure::Get().AddCollider(colliderComponent, *transformComponent);
    }
};
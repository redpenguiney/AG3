#include <cassert>
#include <cstddef>
#include <cstdio>
#include "gameobject.hpp"

std::shared_ptr<GameObject> GameObject::New(const CreateGameObjectParams& params) {
    auto rawPtr = new GameObject(params);
    auto ptr = std::shared_ptr<GameObject>(rawPtr);
    ptr->colliderComponent->gameobject = ptr;
    GAMEOBJECTS.emplace(rawPtr, ptr);
    return ptr;
}    

void GameObject::Destroy() {
    assert(!deleted);
    deleted = true;
    GAMEOBJECTS.erase(this);
}

GameObject::~GameObject() {
    renderComponent->Destroy();
    transformComponent->Destroy();
    colliderComponent->Destroy();
    if (pointLightComponent != nullptr) {
        delete pointLightComponent;
    }
    
    GAMEOBJECTS.erase(this); 
};

void GameObject::Cleanup() {
    std::vector<std::shared_ptr<GameObject>> addresses; // can't remove them inside the map iteration because that would mess up the map iterator
    for (auto & [key, gameobject] : GAMEOBJECTS) {
        (void)key;
        addresses.push_back(gameobject);
    }
    for (auto & gameobject: addresses) {
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
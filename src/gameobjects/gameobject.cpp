#include <cassert>
#include <cstddef>
#include <cstdio>
#include "gameobject.hpp"

std::shared_ptr<GameObject> GameObject::New(unsigned int meshId, unsigned int textureId, bool haveCollisions, bool havePhysics) {
    auto rawPtr = new GameObject(meshId, textureId, haveCollisions, havePhysics);
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

GameObject::GameObject(unsigned int meshId, unsigned int textureId, bool haveCollider, bool havePhysics):
    renderComponent(GraphicsEngine::RenderComponent::New(meshId, textureId)),
    transformComponent(TransformComponent::New()),
    colliderComponent(SpatialAccelerationStructure::ColliderComponent::New(nullptr)) // it needs a shared_ptr so we need to set that in New()
{
    deleted = false;
    colliderComponent->live = haveCollider;
    if (haveCollider) {
        SpatialAccelerationStructure::Get().AddCollider(colliderComponent, *transformComponent);
    }
};
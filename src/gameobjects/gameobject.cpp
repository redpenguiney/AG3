#pragma once
#include <memory>
#include<unordered_set>
#include "../graphics/engine.cpp"
#include "transform_component.cpp"

class GameObject;
std::unordered_set<GameObject*> GAMEOBJECTS;

// The gameobject system uses ECS (google it).
class GameObject {
    public:

    // all gameobjects will reserve a render, transform, collision, and physics component even if they don't need them to ensure cache stuff
        // TODO: this might not be neccesary
    // GraphicsEngine and PhysicsEngine rely on this behavior, DO NOT mess with it
    GraphicsEngine::RenderComponent* const renderComponent;
    TransformComponent* const transformComponent;

    static std::shared_ptr<GameObject> New(unsigned int meshId, unsigned int textureId, bool collides = true) {
        return std::shared_ptr<GameObject>(new GameObject(meshId, textureId));
    }    

    private:
        // no copy constructing gameobjects.
        GameObject(const GameObject&) = delete; 

        GameObject(unsigned int meshId, unsigned int textureId):
        renderComponent(GraphicsEngine::RenderComponent::New(meshId, textureId)),
        transformComponent(TransformComponent::New()) {
            GAMEOBJECTS.insert(this);
        };

        friend class std::shared_ptr<GameObject>;
        ~GameObject() {
            renderComponent->Destroy();
            transformComponent->Destroy();
            GAMEOBJECTS.erase(this); 
        };
};
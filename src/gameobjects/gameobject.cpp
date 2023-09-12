#pragma once
#include<unordered_set>
#include "../graphics/engine.cpp"
#include "transform_component.cpp"

class GameObject;
std::unordered_set<GameObject*> GAMEOBJECTS;

// The gameobject system uses ECS (google it).
class GameObject {
    public:

    // all gameobjects will reserve a render, transform and physics(?) component even if they don't need them to ensure cache stuff
    // GraphicsEngine replies on this behavior, DO NOT mess with it
    GraphicsEngine::RenderComponent* renderComponent;
    TransformComponent* transformComponent;

    static GameObject* New(unsigned int meshId) {
        return new GameObject(meshId);
    }    

    private:
        // no copy constructing gameobjects.
        GameObject(const GameObject&) = delete; 

        GameObject(unsigned int meshId) {
            renderComponent = GraphicsEngine::RenderComponent::New(meshId);
            transformComponent = TransformComponent::New();
            GAMEOBJECTS.insert(this);
        };

        ~GameObject() {
            renderComponent->Destroy();
            transformComponent->Destroy();
            GAMEOBJECTS.erase(this); 
        };
};
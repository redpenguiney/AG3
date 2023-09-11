#pragma once
#include<unordered_set>
#include "../graphics/engine.cpp"
#include "transform_component.cpp"

class GameObject;
std::unordered_set<GameObject*> GAMEOBJECTS;

// The gameobject system uses ECS (google it).
// all gameobjects will reserve a render, transform and physics(?) component even if they don't need them to ensure cache stuff
class GameObject {
    public:
    GraphicsEngine::RenderComponent* renderComponent;
    TransformComponent* transformComponent;

    GameObject* New() {
        return new GameObject();
    }    

    private:
        // no copy constructing gameobjects.
        GameObject(const GameObject&) = delete; 

        GameObject() {
            renderComponent = GraphicsEngine::RenderComponent::New();
            transformComponent = TransformComponent::New();
            GAMEOBJECTS.insert(this);
        };

        ~GameObject() {
            renderComponent->Destroy();
            transformComponent->Destroy();
            GAMEOBJECTS.erase(this); 
        };
};
#pragma once
#include<unordered_set>

class GameObject;
std::unordered_set<GameObject*> GAMEOBJECTS;

// The gameobject system uses ECS (google it).
// 
class GameObject {
    public:
    GameObject* New() {
        return new GameObject();
    }    

    private:
        // no copy constructing gameobjects.
        GameObject(const GameObject&) = delete; 

        GameObject() {
            GAMEOBJECTS.insert(this);
        };
        ~GameObject() {
            GAMEOBJECTS.erase(this);
        };
};
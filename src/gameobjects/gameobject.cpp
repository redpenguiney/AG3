#pragma once
#include<unordered_set>

class GameObject;
std::unordered_set<GameObject*> GAMEOBJECTS;

class GameObject {
    private:
        GameObject() {
            GAMEOBJECTS.insert(this);
        };

        ~GameObject() {
            GAMEOBJECTS.erase(this);
        };
};
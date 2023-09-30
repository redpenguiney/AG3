#pragma once
#include "../gameobjects/component_pool.cpp"
#include "../gameobjects/collider_component.cpp"
#include <memory>

// it's a physics engine, obviously.
class PhysicsEngine {
    public:
    PhysicsEngine(PhysicsEngine const&) = delete; // no copying
    PhysicsEngine& operator=(PhysicsEngine const&) = delete; // no assigning

    static PhysicsEngine& Get()
    {
        static PhysicsEngine engine; // yeah apparently you can have local static variables
        return engine;
    }

    private:
    

    PhysicsEngine() {

    }

    ~PhysicsEngine() {

    }
};
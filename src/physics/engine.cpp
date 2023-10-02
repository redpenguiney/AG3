#pragma once
#include "../gameobjects/component_pool.cpp"
#include "spatial_acceleration_structure.cpp"
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

    // Moves the physics simulation forward by timestep.
    void Step(const float timestep) {

    }

    private:

    PhysicsEngine() {

    }

    ~PhysicsEngine() {

    }
};
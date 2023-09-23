#pragma once
#include "../gameobjects/component_pool.cpp"
#include "../gameobjects/collider_component.cpp"
#include <memory>

// This is a test of doing a good singleton. If this works out then TODO convert graphics engine to be like this.
// it's also a physics engine, obviously.
class PhysicsEngine {
    public:
    PhysicsEngine(PhysicsEngine const&) = delete;
    PhysicsEngine& operator=(PhysicsEngine const&) = delete;

    static std::shared_ptr<PhysicsEngine> instance()
    {
        static std::shared_ptr<PhysicsEngine> engine{new PhysicsEngine};
        return engine;
    }

    private:
    ComponentPool<ColliderComponent, 65536> physicsComponents;

    PhysicsEngine() {

    }
};
#include "../gameobjects/component_pool.hpp"
#include "../gameobjects/rigidbody_component.hpp"
#include "spatial_acceleration_structure.hpp"
#include "../gameobjects/component_registry.hpp"
#include <memory>
#include "engine.hpp"

PhysicsEngine::PhysicsEngine() {}
PhysicsEngine::~PhysicsEngine() {}

PhysicsEngine& PhysicsEngine::Get() {
        static PhysicsEngine engine; // yeah apparently you can have local static variables
        return engine;
}

// Simulates physics of a single rigidbody.
void DoPhysics(SpatialAccelerationStructure::ColliderComponent& collider, TransformComponent& transform, RigidbodyComponent& rigidbody) {
    rigidbody.velocity += glm::dvec3 {0.0, -0.01, 0.0};
}

void PhysicsEngine::Step(const float timestep) {

    

    // iterate through all sets of colliderComponent + rigidBodyComponent + transformComponent
    auto components = ComponentRegistry::GetSystemComponents<TransformComponent, SpatialAccelerationStructure::ColliderComponent, RigidbodyComponent>();

    // first pass, do collisions and what not for non-kinematic objects
    for (auto & tuple: components) {
        TransformComponent& transform = std::get<0>(tuple);
        SpatialAccelerationStructure::ColliderComponent& collider = std::get<1>(tuple);
        RigidbodyComponent& rigidbody = std::get<2>(tuple);
        if (collider.live && !rigidbody.kinematic) {
            DoPhysics(collider, transform, rigidbody);
        }
    }

    // second pass, move everything by its velocity
    for (auto & tuple: components) {
        TransformComponent& transform = std::get<0>(tuple);
        SpatialAccelerationStructure::ColliderComponent& collider = std::get<1>(tuple);
        RigidbodyComponent& rigidbody = std::get<2>(tuple);
        if (collider.live) {
            transform.SetPos(transform.position() + rigidbody.velocity);
        }
    }
}
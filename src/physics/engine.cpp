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
    transform.SetPos(transform.position() + rigidbody.velocity);
}

void PhysicsEngine::Step(const float timestep) {

    // iterate through all sets of colliderComponent + rigidBodyComponent + transformComponent
    auto pools = ComponentRegistry::GetSystemComponents({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex, ComponentRegistry::RigidbodyComponentBitIndex});

    for (auto & poolVec : pools) {
        ComponentPool<TransformComponent>* transforms = (ComponentPool<TransformComponent>*)poolVec[ComponentRegistry::TransformComponentBitIndex];
        ComponentPool<SpatialAccelerationStructure::ColliderComponent>* colliders = (ComponentPool<SpatialAccelerationStructure::ColliderComponent>*)poolVec[ComponentRegistry::ColliderComponentBitIndex];
        ComponentPool<RigidbodyComponent>* rigidbodies = (ComponentPool<RigidbodyComponent>*)poolVec[ComponentRegistry::RigidbodyComponentBitIndex];
        
        for (unsigned int i = 0; i < colliders->pools.size(); i++) {
            auto colliderArray = colliders->pools.at(i);
            auto transformArray = transforms->pools.at(i);
            auto rigidBodyArray = rigidbodies->pools.at(i);
            for (unsigned int j = 0; j < colliders->COMPONENTS_PER_POOL; j++) {
                auto collider = colliderArray + j;
                auto transform = transformArray + j;
                auto rigidbody = rigidBodyArray + j;
                if (collider->live && !rigidbody->kinematic) {
                    DoPhysics(*collider, *transform, *rigidbody);
                }
            }
        }
    }
}
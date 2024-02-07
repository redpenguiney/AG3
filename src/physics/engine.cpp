#include "../gameobjects/component_pool.hpp"
#include "../gameobjects/rigidbody_component.hpp"
#include "spatial_acceleration_structure.hpp"
#include "../gameobjects/component_registry.hpp"
#include <cassert>
#include <cstdio>
#include <memory>
#include "../utility/utility.hpp"
#include <vector>
#include "engine.hpp"
#include "gjk.hpp"
#include "../../external_headers/GLM/gtx/string_cast.hpp"


PhysicsEngine::PhysicsEngine() {
    // GRAVITY = {0, -0.0, 0};
    GRAVITY = {0, -9.807, 0};
}
PhysicsEngine::~PhysicsEngine() {}

PhysicsEngine& PhysicsEngine::Get() {
        static PhysicsEngine engine; // yeah apparently you can have local static variables
        return engine;
}



// Simulates physics of a single rigidbody.
void DoPhysics(const double dt, SpatialAccelerationStructure::ColliderComponent& collider, TransformComponent& transform, RigidbodyComponent& rigidbody, std::vector<std::pair<TransformComponent*, glm::dvec3>>& seperations) {
    rigidbody.accumulatedForce += PhysicsEngine::Get().GRAVITY * dt * (double)rigidbody.mass;

    
    // TODO: should REALLY use tight fitting AABB here
    auto aabbToTest = collider.GetAABB();
    std::vector<SpatialAccelerationStructure::ColliderComponent*> potentialColliding = SpatialAccelerationStructure::Get().Query(aabbToTest);
    // assert(potentialColliding.size() == 0);

    // TODO: potential perf gains by using tight fitting AABB/OBB here after broadphase SAS query?
    
    // we don't do anything to the other gameobjects we hit; if they have a rigidbody, they will independently take care of that in their call to DoPhysics()
    // TODO: this does mean redundant narrowphase collision checks, we should cache results of narrowphase checks to avoid that
    for (auto & otherColliderPtr: potentialColliding) {
        if (otherColliderPtr == &collider) {continue;} // collider shouldn't collide with itself lol
        auto collisionTestResult = IsColliding(*otherColliderPtr->GetGameObject()->transformComponent.GetPtr(), *otherColliderPtr, transform, collider);
        if (collisionTestResult) {
            assert(collisionTestResult->hitPoints.size() > 0);

            // find center of contact region and apply the torque from there
            glm::dvec3 averageContactPoint = {0, 0, 0}; // in object space
            double averagePenetration = 0;
            for (auto & p: collisionTestResult->hitPoints) {
                // std::cout << "One point is " << glm::to_string(p.first) << ".\n";
                averageContactPoint += p.first;
                averagePenetration += p.second;
                // DebugPlacePointOnPosition({p.first + p.second}, {0.5, 0.0, 0.0, 1.0});
            }
            averageContactPoint /= collisionTestResult->hitPoints.size();
            averagePenetration /= collisionTestResult->hitPoints.size();

            // averagePenetration = std::max(0, averagePenetration + 0.01)''
            // std::cout << "Object is at " << glm::to_string(transform.position()) << "\n";
            // std::cout << "COLLISION!!! normal is " << glm::to_string(collisionTestResult->collisionNormal) << " world position " << glm::to_string(collisionTestResult->hitPoints.back().first) << " distance " << averagePenetration << "\n"; 
            
            const ComponentHandle<RigidbodyComponent>& otherRigidbody = otherColliderPtr->GetGameObject()->rigidbodyComponent;

            auto seperationVector = collisionTestResult->collisionNormal * (otherRigidbody ? 0.5 * averagePenetration : averagePenetration);
            DebugPlacePointOnPosition({transform.position() + seperationVector}, {1, 0.2, 0.2, 1.0});
            

            
            DebugPlacePointOnPosition({averageContactPoint}, {0.2, 0.2, 1.0, 1.0});

            // collision impulse-based response stolen from https://physics.stackexchange.com/questions/686640/resolving-angular-components-in-2d-circular-rigid-body-collision-response

            // velocity of the actual point that hit should be used, not the overall velocity of the object
            // glm::vec3 contactPointInObjectSpace = glm::dvec3(glm::inverse(transform.GetPhysicsModelMatrix()) * glm::dvec4(averageContactPoint, 1)); // TODO: might need to use whole inverted matrix
            
            
            glm::vec3 contactPointInObjectSpace = averageContactPoint - transform.position();

            glm::dvec3 velocityAtContactPoint = rigidbody.velocity + rigidbody.VelocityAtPoint(contactPointInObjectSpace);
            glm::vec3 contactPointInOtherObjectSpace = averageContactPoint - otherColliderPtr->GetGameObject()->transformComponent->position(); // TODO: might need to use whole inverted matrix
            // glm::dvec3 velocityAtOtherContactPoint = otherColliderPtr->GetGameObject()->rigidbodyComponent ? otherColliderPtr->GetGameObject()->rigidbodyComponent->velocity :  glm::dvec3(0, 0, 0);
            glm::dvec3 velocityAtOtherContactPoint = otherRigidbody ?  (otherRigidbody->velocity + otherRigidbody->VelocityAtPoint(contactPointInOtherObjectSpace)) : glm::dvec3(0, 0, 0);

            seperations.emplace_back(std::make_pair(&transform, seperationVector));
            // std::cout << "Seperating by " << glm::to_string(seperationVector) << ".\n";
            auto elasticity = otherColliderPtr->elasticity * collider.elasticity;
            // std::cout << "Elasticity = " << (otherColliderPtr->GetGameObject()->rigidbodyComponent ? elasticity : 1.0 + elasticity) << ".\n";
            
            
            auto relVelocity = (velocityAtOtherContactPoint - velocityAtContactPoint); // TODO: abs might create weird behaviour idk
            // std::cout << "Rigidbody had velocity " << glm::to_string(rigidbody.velocity) << ",\n";
            // ???
            glm::vec3 crossThing1 = glm::cross((glm::vec3)collisionTestResult->collisionNormal, contactPointInObjectSpace);
            glm::vec3 crossThing2 = glm::cross((glm::vec3)collisionTestResult->collisionNormal, contactPointInOtherObjectSpace);
            double reducedMass = 1/(1/rigidbody.mass + (otherRigidbody ? 1/otherRigidbody->mass : 0) + glm::dot(crossThing1, (crossThing1/rigidbody.momentOfInertia)) + glm::dot(crossThing2, otherRigidbody ? (crossThing2/otherRigidbody->momentOfInertia): glm::vec3(0, 0, 0)));
            // std::cout << "Reduced mass is " << reducedMass << ".\n";
            glm::vec3 impulse = (1 + elasticity) * reducedMass * relVelocity;
            // std::cout << "Impulse is " << glm::to_string(impulse) << ".\n";
            // std::cout << "Normal is " << glm::to_string(collisionTestResult->collisionNormal) << ".\n";
            rigidbody.accumulatedForce += impulse * glm::vec3(collisionTestResult->collisionNormal);
            rigidbody.accumulatedTorque -= glm::cross(-contactPointInObjectSpace, glm::vec3(collisionTestResult->collisionNormal)) * glm::length(impulse);

            // we want a specific POINT to have this change in velocity.
            // auto desiredChangeInVelocity = (otherColliderPtr->GetGameObject()->rigidbodyComponent ? elasticity : 1.0 + elasticity) * collisionTestResult->collisionNormal * relVelocity; //* (glm::dot(collisionTestResult->collisionNormal, relVelocity) / glm::dot(collisionTestResult->collisionNormal, collisionTestResult->collisionNormal));
            // rigidbody.accumulatedForce = desiredChangeInVelocity * (double)rigidbody.mass;

            // torque = cross(radius, force)
            // std::cout << "angular velocity is " << glm::to_string(rigidbody.angularVelocity) << " so velocity at point is " << glm::to_string((glm::dvec3)(glm::cross(rigidbody.angularVelocity, contactPointInObjectSpace) * contactPointInObjectSpace)) << ".\n";
            // std::cout << " Contact point in object space is " << glm::to_string(contactPointInObjectSpace) << " \n";
            // std::cout << " Delta-v is " << glm::to_string(desiredChangeInVelocity) << " \n";
            // std::cout << "cross product " << glm::to_string(glm::cross(-contactPointInObjectSpace, glm::vec3(collisionTestResult->collisionNormal))) << " \n"; 
            // rigidbody.accumulatedTorque += glm::cross(contactPointInObjectSpace, impulse * glm::vec3(collisionTestResult->collisionNormal));
            // rigidbody.Impulse(contactPointInObjectSpace, (glm::vec3)desiredChangeInVelocity * rigidbody.mass);
            // while (true) {}
        }
        else {
            //std::cout << "aw.\n";
        }
    }
}

void PhysicsEngine::Step(const double timestep) {

    

    // iterate through all sets of colliderComponent + rigidBodyComponent + transformComponent
    auto components = ComponentRegistry::GetSystemComponents<TransformComponent, SpatialAccelerationStructure::ColliderComponent, RigidbodyComponent>();

    // first pass, convert applied force to velocity, apply drag, and move everything by its velocity
    for (auto & tuple: components) {
        TransformComponent& transform = *std::get<0>(tuple);
        SpatialAccelerationStructure::ColliderComponent& collider = *std::get<1>(tuple);
        RigidbodyComponent& rigidbody = *std::get<2>(tuple);
        if (collider.live) {
            // TODO: drag ignores timestep
            rigidbody.velocity *= rigidbody.linearDrag;
            rigidbody.angularVelocity *= rigidbody.angularDrag;

            // f = ma, so a = f/m
            
            rigidbody.velocity += rigidbody.accumulatedForce/rigidbody.mass;
            rigidbody.accumulatedForce = {0, 0, 0};

            // a = t/i
            rigidbody.angularVelocity += rigidbody.accumulatedTorque/rigidbody.momentOfInertia;
            rigidbody.accumulatedTorque = {0, 0, 0};

            if (rigidbody.velocity != glm::dvec3(0, 0, 0)) {
                transform.SetPos(transform.position() + rigidbody.velocity * timestep);
            }
           
            if (rigidbody.angularVelocity != glm::vec3(0, 0, 0)) {
                glm::quat QuatAroundX = glm::angleAxis(rigidbody.angularVelocity.x * (float)timestep, glm::vec3(1.0,0.0,0.0));
                glm::quat QuatAroundY = glm::angleAxis( rigidbody.angularVelocity.y * (float)timestep, glm::vec3(0.0,1.0,0.0));
                glm::quat QuatAroundZ = glm::angleAxis( rigidbody.angularVelocity.z * (float)timestep, glm::vec3(0.0,0.0,1.0));
                glm::quat finalOrientation = QuatAroundX * QuatAroundY * QuatAroundZ;
                //std::cout << "velocity " << glm::to_string(rigidbody.momentOfInertia) << " so we at " << glm::to_string(QuatAroundX) << " \n";
                transform.SetRot(transform.rotation() * finalOrientation);
            }
        }
    }
    
    // second pass, do collisions and constraints for non-kinematic objects
    // the second pass should under no circumstances change any property of the gameobjects, except for the accumulatedForce of the object being moved, so that the order of operations doesn't matter and so that physics can be parallelized.
    
    // for each pair, the transform component will be shifted over by the vector
    std::vector<std::pair<TransformComponent*, glm::dvec3>> separations; // to separate colliding objects, since we can't change position in this pass, DoPhysics() adds desired translations to this std::vector, and 3rd pass actually sets position 
    
    for (auto & tuple: components) {
        TransformComponent& transform = *std::get<0>(tuple);
        SpatialAccelerationStructure::ColliderComponent& collider = *std::get<1>(tuple);
        RigidbodyComponent& rigidbody = *std::get<2>(tuple);
        if (collider.live && !rigidbody.kinematic) {
            DoPhysics(timestep, collider, transform, rigidbody, separations);
        }
    }

    // third pass, seperate colliding objects since we couldn't change positions in 2nd pass
    for (auto & [comp, offset]: separations) {
        comp->SetPos(comp->position() + offset);
    }
}
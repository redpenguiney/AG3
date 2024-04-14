#include "../gameobjects/component_pool.hpp"
#include "../gameobjects/rigidbody_component.hpp"
#include "spatial_acceleration_structure.hpp"
#include "../gameobjects/component_registry.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include "../utility/utility.hpp"
#include <vector>
#include "engine.hpp"
#include "gjk.hpp"
#include "../../external_headers/GLM/gtx/string_cast.hpp"


PhysicsEngine::PhysicsEngine() {
    // GRAVITY = {0, -0.0, 0};
    GRAVITY = {0, -9, 0};
}
PhysicsEngine::~PhysicsEngine() {}

PhysicsEngine& PhysicsEngine::Get() {
        static PhysicsEngine engine; // yeah apparently you can have local static variables
        return engine;
}



// Simulates physics of a single rigidbody.
void DoPhysics(const double dt, SpatialAccelerationStructure::ColliderComponent& collider, TransformComponent& transform, RigidbodyComponent& rigidbody, std::vector<std::pair<TransformComponent*, glm::dvec3>>& seperations) {    
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

            const ComponentHandle<RigidbodyComponent>& otherRigidbody = otherColliderPtr->GetGameObject()->rigidbodyComponent;
            const ComponentHandle<TransformComponent>& otherTransform = otherColliderPtr->GetGameObject()->transformComponent;

            assert(collisionTestResult->contactPoints.size() > 0);

            // find center of contact region and apply the torque from there
            glm::dvec3 averageContactPoint = {0, 0, 0}; // in object space
            double averagePenetration = 0;
            for (auto & p: collisionTestResult->contactPoints) {
                // std::cout << "One point is " << glm::to_string(p.first) << ".\n";
                averageContactPoint += p.first;
                averagePenetration += p.second;
                // DebugPlacePointOnPosition({p.first + p.second}, {0.5, 0.0, 0.0, 1.0});
            }
            averageContactPoint /= collisionTestResult->contactPoints.size();
            averagePenetration /= collisionTestResult->contactPoints.size();
            glm::dvec3 normal = collisionTestResult->collisionNormal; // inverted so it faces out of collider, not otherCollider.
            

            //     // collision impulse-based response stolen from https://physics.stackexchange.com/questions/686640/resolving-angular-components-in-2d-circular-rigid-body-collision-response
            //     // also this source looks useful https://gafferongames.com/post/collision_response_and_coulomb_friction/
            //     // and this https://gamedev.stackexchange.com/questions/131219/rigid-body-physics-resolution-causing-never-ending-bouncing-and-jittering?rq=1

            // std::cout << "COLLISION!!! normal is " << glm::to_string(collisionTestResult->collisionNormal) << " world position " << glm::to_string(collisionTestResult->hitPoints.back().first) << " distance " << averagePenetration << "\n"; 
            const double LINEAR_SLOP = 0.0;
            auto seperationVector = normal * ((otherRigidbody ? 0.5 * averagePenetration : averagePenetration) - LINEAR_SLOP);
                // DebugPlacePointOnPosition({transform.Position() + seperationVector}, {1, 0.2, 0.2, 1.0});
            seperations.emplace_back(std::make_pair(&transform, seperationVector));

                
                DebugPlacePointOnPosition({averageContactPoint}, {0.2, 0.2, 1.0, 1.0});


                // velocity of the actual point that hit should be used, not the overall velocity of the object
                // glm::vec3 contactPointInObjectSpace = glm::dvec3(glm::inverse(transform.GetPhysicsModelMatrix()) * glm::dvec4(averageContactPoint, 1)); // TODO: might need to use whole inverted matrix
                
                
            glm::vec3 contactToRigidbody = (transform.Position() - averageContactPoint);

            glm::dvec3 velocityAtContactPoint = rigidbody.velocity + rigidbody.VelocityAtPoint(contactToRigidbody);
            glm::vec3 contactToOtherRigidbody = (otherTransform->Position() - averageContactPoint); // TODO: might need to use whole inverted matrix
            // glm::dvec3 velocityAtOtherContactPoint = otherColliderPtr->GetGameObject()->rigidbodyComponent ? otherColliderPtr->GetGameObject()->rigidbodyComponent->velocity :  glm::dvec3(0, 0, 0);
            glm::dvec3 velocityAtOtherContactPoint = otherRigidbody ?  (otherRigidbody->velocity + otherRigidbody->VelocityAtPoint(contactToOtherRigidbody)) : glm::dvec3(0, 0, 0);
   
            // std::cout << "Seperating by " << glm::to_string(seperationVector) << ".\n";
            auto elasticity = otherColliderPtr->elasticity * collider.elasticity;
            // auto density = otherColliderPtr->density * collider.density;
            // if (collisionTestResult->contactPoints.size() < 4) {
                // elasticity += 0.2;
            // }
            // std::cout << "Elasticity = " << (1.0 + elasticity) << ".\n";
            
            
            auto relVelocity = (velocityAtOtherContactPoint - velocityAtContactPoint); // TODO: abs might create weird behaviour idk
            double relVelocityAlongContactNormal = glm::dot(glm::dvec3(normal), relVelocity);
            std::cout << "Rigidbody had velocity " << glm::to_string(rigidbody.velocity) << ", at point its " << glm::to_string(rigidbody.VelocityAtPoint(contactToRigidbody)) << ",\n";
            std::cout << " Rel vel " << glm::to_string(relVelocity) << " along normal " << relVelocityAlongContactNormal << ".\n";
            // if (relVelocityAlongContactNormal < 0) {
            //     std::cout << "WARNING::: WE DOING NOTHING!!!\n\n";
            //     continue;
            // }
            
            float reducedMass = rigidbody.InverseMass() + (otherRigidbody ? otherRigidbody->InverseMass() : 0); 
            glm::vec3 ncr = glm::cross(glm::vec3(normal), contactToRigidbody);
            glm::vec3 globalMmoiInverse = {3.0, 3.0, 3.0}; // rigidbody.GetInverseGlobalMomentOfInertia(transform)
            reducedMass -= glm::dot(glm::vec3(normal), glm::cross(globalMmoiInverse * ncr, contactToRigidbody));
            reducedMass = 1.0f/reducedMass;
            assert(reducedMass <= 1/rigidbody.InverseMass());

            float impulse =  -(1 + elasticity) * relVelocityAlongContactNormal / reducedMass;

            auto collisionForce = -(impulse * glm::vec3(normal)); 
            rigidbody.accumulatedForce += collisionForce;
            rigidbody.accumulatedTorque += glm::cross(contactToRigidbody, glm::vec3(normal)) * impulse;

            std::cout << "Accumulated force is now " << glm::to_string(rigidbody.accumulatedForce) << ". Impulse is " << impulse << ". Reduced mass is " << reducedMass << ".\n";
            // }

            // FRICTION
            glm::vec3 relVelocityAlongPlane =  relVelocity + (normal * relVelocityAlongContactNormal);
            if (glm::length(relVelocityAlongPlane) > 0) {
                auto tangentDirection = glm::normalize(relVelocityAlongPlane);

                glm::vec3 planarCrossThing1 = glm::cross((glm::vec3)tangentDirection, contactToRigidbody);
                glm::vec3 planarCrossThing2 = glm::cross((glm::vec3)tangentDirection, contactToOtherRigidbody);
                // std::cout << "Along plane, rel velocity is " << glm::to_string(relVelocityAlongPlane) << " as calculated from total rel vel " << glm::to_string(relVelocity) << " and vel along normal " << relVelocityAlongContactNormal << ".\n";
                
                float friction = otherColliderPtr->friction * collider.friction;
                // double reducedInverseMassAlongPlane = 1/(rigidbody.InverseMass() + (otherRigidbody ? otherRigidbody->InverseMass() : 0) + glm::dot(planarCrossThing1, (globalInverseInertiaTensor1 * planarCrossThing1)) + glm::dot(planarCrossThing2, otherRigidbody ? (globalInverseInertiaTensor2 * planarCrossThing2): glm::vec3(0, 0, 0)));
                // float frictionImpulse = friction * reducedInverseMassAlongPlane * glm::length(relVelocityAlongPlane);

                // if (glm::length(relVelocityAlongPlane) > friction * )
                // rigidbody.accumulatedForce += frictionImpulse * tangentDirection;   
                // rigidbody.accumulatedTorque -= glm::cross(-posRelToContact, -tangentDirection) * frictionImpulse;
            }

        }
        else {
            //std::cout << "aw.\n";
        }
    }
}

void PhysicsEngine::Step(const double timestep) {

    

    // iterate through all sets of colliderComponent + rigidBodyComponent + transformComponent
    auto components = ComponentRegistry::GetSystemComponents<TransformComponent, SpatialAccelerationStructure::ColliderComponent, RigidbodyComponent>();

    // first pass, apply gravity, convert applied force to velocity, apply drag, and move everything by its velocity
    for (auto & tuple: components) {
        TransformComponent& transform = *std::get<0>(tuple);
        SpatialAccelerationStructure::ColliderComponent& collider = *std::get<1>(tuple);
        RigidbodyComponent& rigidbody = *std::get<2>(tuple);
        if (collider.live) {
            // transform.SetRot(glm::normalize(transform.Rotation()));
            rigidbody.accumulatedForce += PhysicsEngine::Get().GRAVITY * timestep * (double)rigidbody.InverseMass();

            // TODO: drag ignores timestep
            // rigidbody.velocity *= rigidbody.linearDrag;
            // rigidbody.angularVelocity *= rigidbody.angularDrag;

            // f = ma, so a = f/m
            
            rigidbody.velocity += rigidbody.accumulatedForce * rigidbody.InverseMass();
            rigidbody.accumulatedForce = {0, 0, 0};

            // a = t/i
            // glm::mat3 globalInverseInertiaTensor = rigidbody.GetInverseGlobalMomentOfInertia(transform); 
            // rigidbody.angularVelocity += globalInverseInertiaTensor * rigidbody.accumulatedTorque;
            rigidbody.angularVelocity += glm::inverse(rigidbody.localMomentOfInertia) * rigidbody.accumulatedTorque;
            rigidbody.accumulatedTorque = {0, 0, 0};

            if (rigidbody.velocity != glm::dvec3(0, 0, 0)) {
                transform.SetPos(transform.Position() + rigidbody.velocity * timestep);
            }
           
            if (rigidbody.angularVelocity != glm::vec3(0, 0, 0)) {
                glm::quat QuatAroundX = glm::angleAxis(rigidbody.angularVelocity.x * (float)timestep, glm::vec3(1.0,0.0,0.0));
                glm::quat QuatAroundY = glm::angleAxis( rigidbody.angularVelocity.y * (float)timestep, glm::vec3(0.0,1.0,0.0));
                glm::quat QuatAroundZ = glm::angleAxis( rigidbody.angularVelocity.z * (float)timestep, glm::vec3(0.0,0.0,1.0));
                glm::quat finalOrientation = QuatAroundX * QuatAroundY * QuatAroundZ;
                //std::cout << "velocity " << glm::to_string(rigidbody.localMomentOfInertia) << " so we at " << glm::to_string(QuatAroundX) << " \n";
                transform.SetRot(transform.Rotation() * finalOrientation);
            }
        }
    }
    
    // second pass, do collisions and constraints for non-kinematic objects
    // the second pass should under no circumstances change any property of the gameobjects, except for the accumulatedForce of the object being moved, so that the order of operations doesn't matter and so that physics can be parallelized.
    
    // for each pair, the transform component will be shifted over by the vector
    // TODO: NOT THREAD SAFE MY BAD DO NOT FORGET TO FIX
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
        comp->SetPos(comp->Position() + offset);
    }
}
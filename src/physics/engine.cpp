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
            glm::dvec3 normal = -collisionTestResult->collisionNormal; // inverted so it faces out of collider, not otherCollider.
            

            // float totalMultiplier = 0;
            // for (auto & [contactPoint, penetration]: collisionTestResult->contactPoints) {
            //     // auto seperationVector = collisionTestResult->collisionNormal * (otherRigidbody ? 0.5 * penetration : penetration);
            //     // DebugPlacePointOnPosition({transform.position() + seperationVector}, {1, 0.2, 0.2, 1.0});
                

                
            //     // DebugPlacePointOnPosition({averageContactPoint}, {0.2, 0.2, 1.0, 1.0});

            //     // collision impulse-based response stolen from https://physics.stackexchange.com/questions/686640/resolving-angular-components-in-2d-circular-rigid-body-collision-response

            //     // velocity of the actual point that hit should be used, not the overall velocity of the object
            //     // glm::vec3 contactPointInObjectSpace = glm::dvec3(glm::inverse(transform.GetPhysicsModelMatrix()) * glm::dvec4(averageContactPoint, 1)); // TODO: might need to use whole inverted matrix
                
                
            //     glm::vec3 contactPointInObjectSpace = contactPoint - transform.position();

            //     glm::dvec3 velocityAtContactPoint = rigidbody.velocity + rigidbody.VelocityAtPoint(contactPointInObjectSpace);
            //     glm::vec3 contactPointInOtherObjectSpace = contactPoint - otherTransform->position(); // TODO: might need to use whole inverted matrix
            //     // glm::dvec3 velocityAtOtherContactPoint = otherColliderPtr->GetGameObject()->rigidbodyComponent ? otherColliderPtr->GetGameObject()->rigidbodyComponent->velocity :  glm::dvec3(0, 0, 0);
            //     glm::dvec3 velocityAtOtherContactPoint = otherRigidbody ?  (otherRigidbody->velocity + otherRigidbody->VelocityAtPoint(contactPointInOtherObjectSpace)) : glm::dvec3(0, 0, 0);

            //     // seperations.emplace_back(std::make_pair(&transform, seperationVector));
            //     // std::cout << "Seperating by " << glm::to_string(seperationVector) << ".\n";
            //     auto elasticity = otherColliderPtr->elasticity * collider.elasticity;
            //     std::cout << "Elasticity = " << (1.0 + elasticity) << ".\n";
                
                
            //     auto relVelocity = (velocityAtOtherContactPoint - velocityAtContactPoint); // TODO: abs might create weird behaviour idk
            //     // std::cout << "Rigidbody had velocity " << glm::to_string(rigidbody.velocity) << ",\n";
            //     // ???
            //     glm::vec3 crossThing1 = glm::cross((glm::vec3)collisionTestResult->collisionNormal, contactPointInObjectSpace);
            //     glm::vec3 crossThing2 = glm::cross((glm::vec3)collisionTestResult->collisionNormal, contactPointInOtherObjectSpace);
            //     double reducedMass = 1/(1/rigidbody.mass + (otherRigidbody ? 1/otherRigidbody->mass : 0) + glm::dot(crossThing1, (crossThing1/rigidbody.localMomentOfInertia)) + glm::dot(crossThing2, otherRigidbody ? (crossThing2/otherRigidbody->localMomentOfInertia): glm::vec3(0, 0, 0)));
            //     // std::cout << "Reduced mass is " << reducedMass << ".\n";
            //     glm::vec3 impulse = (1 + elasticity) * reducedMass * relVelocity / (double)collisionTestResult->contactPoints.size(); //* (penetration/averagePenetration);
            //     // totalMultiplier += (reducedMass);
            //     std::cout << "Impulse is " << glm::to_string(impulse) << ".\n";
            //     std::cout << "Normal is " << glm::to_string(collisionTestResult->collisionNormal) << " rel velocity " << glm::to_string(relVelocity) << ".\n";
            //     rigidbody.accumulatedForce += (impulse * glm::vec3(collisionTestResult->collisionNormal));// (float)collisionTestResult->contactPoints.size();
            //     rigidbody.accumulatedTorque -= glm::cross(-contactPointInObjectSpace, glm::vec3(collisionTestResult->collisionNormal)) * glm::length(impulse);
            // }

            // std::cout << "Total reduced mass was " << totalMultiplier << ".\n";
            // averagePenetration = std::max(0, averagePenetration + 0.01)''
            // std::cout << "Object is at " << glm::to_string(transform.position()) << "\n";
            // std::cout << "COLLISION!!! normal is " << glm::to_string(collisionTestResult->collisionNormal) << " world position " << glm::to_string(collisionTestResult->hitPoints.back().first) << " distance " << averagePenetration << "\n"; 
            const double LINEAR_SLOP = 0.0;
            auto seperationVector = -normal * ((otherRigidbody ? 0.5 * averagePenetration : averagePenetration) - LINEAR_SLOP);
                // DebugPlacePointOnPosition({transform.position() + seperationVector}, {1, 0.2, 0.2, 1.0});
            seperations.emplace_back(std::make_pair(&transform, seperationVector));

                
                DebugPlacePointOnPosition({averageContactPoint}, {0.2, 0.2, 1.0, 1.0});

                // collision impulse-based response stolen from https://physics.stackexchange.com/questions/686640/resolving-angular-components-in-2d-circular-rigid-body-collision-response

                // velocity of the actual point that hit should be used, not the overall velocity of the object
                // glm::vec3 contactPointInObjectSpace = glm::dvec3(glm::inverse(transform.GetPhysicsModelMatrix()) * glm::dvec4(averageContactPoint, 1)); // TODO: might need to use whole inverted matrix
                
                
            glm::vec3 posRelToContact = transform.position() - averageContactPoint;

            glm::dvec3 velocityAtContactPoint = rigidbody.velocity + rigidbody.VelocityAtPoint(posRelToContact);
            glm::vec3 otherPosRelToContact = otherTransform->position() - averageContactPoint; // TODO: might need to use whole inverted matrix
            // glm::dvec3 velocityAtOtherContactPoint = otherColliderPtr->GetGameObject()->rigidbodyComponent ? otherColliderPtr->GetGameObject()->rigidbodyComponent->velocity :  glm::dvec3(0, 0, 0);
            glm::dvec3 velocityAtOtherContactPoint = otherRigidbody ?  (otherRigidbody->velocity + otherRigidbody->VelocityAtPoint(otherPosRelToContact)) : glm::dvec3(0, 0, 0);

            
            // std::cout << "Seperating by " << glm::to_string(seperationVector) << ".\n";
            auto elasticity = otherColliderPtr->elasticity * collider.elasticity;
            auto density = otherColliderPtr->density * collider.density;
            if (collisionTestResult->contactPoints.size() < 4) {
                elasticity += 0.2;
            }
            std::cout << "Elasticity = " << (1.0 + elasticity) << ".\n";
            
            
            // auto relVelocity = (velocityAtOtherContactPoint - velocityAtContactPoint); // TODO: abs might create weird behaviour idk
            double relVelocityAlongContactNormal = glm::dot(glm::dvec3(normal), velocityAtContactPoint - velocityAtOtherContactPoint);
            // std::cout << "Rigidbody had velocity " << glm::to_string(rigidbody.velocity) << ",\n";
            
            // ???
            glm::vec3 crossThing1 = glm::cross(-(glm::vec3)normal, -posRelToContact);
            glm::vec3 crossThing2 = glm::cross(-(glm::vec3)normal, -otherPosRelToContact);
            glm::mat3 globalInverseInertiaTensor1 = rigidbody.GetInverseGlobalMomentOfInertia(transform);
            glm::mat3 globalInverseInertiaTensor2 = otherRigidbody ? otherRigidbody->GetInverseGlobalMomentOfInertia(*otherTransform): glm::identity<glm::mat3x3>();
            double reducedMass = 1/(1/rigidbody.mass + (otherRigidbody ? 1/otherRigidbody->mass : 0) + glm::dot(crossThing1, (crossThing1 * globalInverseInertiaTensor1)) + glm::dot(crossThing2, otherRigidbody ? (crossThing2 * globalInverseInertiaTensor2): glm::vec3(0, 0, 0)));
            
            // std::cout << "Reduced mass is " << reducedMass << ".\n";
            float impulse = (1 + elasticity) * reducedMass * relVelocityAlongContactNormal; //* (penetration/averagePenetration);
            std::cout << "Impulse is " << impulse << ".\n";
            std::cout << "Normal is " << glm::to_string(normal) << " rel velocity " << relVelocityAlongContactNormal << ".\n";
            // std::cout << "Prior to impulse accumulated force is " << glm::to_string(rigidbody.accumulatedForce) << ".\n";
            

            // auto penetrationForce = collisionTestResult->collisionNormal * (std::max(0.0, averagePenetration - LINEAR_SLOP) * density);
            // if (glm::length2(impactForce) < glm::length2(penetrationForce)) {
            // if (glm::length(relVelocity) < 20) {
            //     rigidbody.accumulatedForce += penetrationForce;
            //     rigidbody.accumulatedTorque -= rigidbody.angularVelocity * float(std::max(0.0, averagePenetration - LINEAR_SLOP) * density);
            //     std::cout << "PENETRATE\n";
            // }
            // else {
                // std::cout << "IMPULSE\n";
            rigidbody.accumulatedForce -= (impulse * glm::vec3(normal));
            
            rigidbody.accumulatedTorque -= glm::cross(-posRelToContact, glm::vec3(normal)) * impulse;
            // }
            
            // std::cout << "It SHOULD have apllied force " << glm::to_string(impulse * glm::vec3(collisionTestResult->collisionNormal)) << ".\n";


            // we want a specific POINT to have this change in velocity.
            // auto desiredChangeInVelocity = (otherColliderPtr->GetGameObject()->rigidbodyComponent ? elasticity : 1.0 + elasticity) * collisionTestResult->collisionNormal * relVelocity; //* (glm::dot(collisionTestResult->collisionNormal, relVelocity) / glm::dot(collisionTestResult->collisionNormal, collisionTestResult->collisionNormal));
            // rigidbody.accumulatedForce = desiredChangeInVelocity * (double)rigidbody.mass;

            // torque = cross(radius, force)
            // std::cout << "angular velocity is " << glm::to_string(rigidbody.angularVelocity) << " so velocity at point is " << glm::to_string((glm::dvec3)(glm::cross(rigidbody.angularVelocity, contactPointInObjectSpace) * contactPointInObjectSpace)) << ".\n";
            // std::cout << " Contact point in object space is " << glm::to_string(contactPointInObjectSpace) << " \n";
            // std::cout << "Force applied is " << glm::to_string(rigidbody.accumulatedForce) << ".\n";
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

    // first pass, apply gravity, convert applied force to velocity, apply drag, and move everything by its velocity
    for (auto & tuple: components) {
        TransformComponent& transform = *std::get<0>(tuple);
        SpatialAccelerationStructure::ColliderComponent& collider = *std::get<1>(tuple);
        RigidbodyComponent& rigidbody = *std::get<2>(tuple);
        if (collider.live) {
            rigidbody.accumulatedForce += PhysicsEngine::Get().GRAVITY * timestep * (double)rigidbody.mass;

            // TODO: drag ignores timestep
            // rigidbody.velocity *= rigidbody.linearDrag;
            // rigidbody.angularVelocity *= rigidbody.angularDrag;

            // f = ma, so a = f/m
            
            rigidbody.velocity += rigidbody.accumulatedForce/rigidbody.mass;
            rigidbody.accumulatedForce = {0, 0, 0};

            // a = t/i
            glm::mat3 globalInverseInertiaTensor = rigidbody.GetInverseGlobalMomentOfInertia(transform); 
            rigidbody.angularVelocity += globalInverseInertiaTensor * rigidbody.accumulatedTorque;
            rigidbody.accumulatedTorque = {0, 0, 0};

            if (rigidbody.velocity != glm::dvec3(0, 0, 0)) {
                transform.SetPos(transform.position() + rigidbody.velocity * timestep);
            }
           
            if (rigidbody.angularVelocity != glm::vec3(0, 0, 0)) {
                glm::quat QuatAroundX = glm::angleAxis(rigidbody.angularVelocity.x * (float)timestep, glm::vec3(1.0,0.0,0.0));
                glm::quat QuatAroundY = glm::angleAxis( rigidbody.angularVelocity.y * (float)timestep, glm::vec3(0.0,1.0,0.0));
                glm::quat QuatAroundZ = glm::angleAxis( rigidbody.angularVelocity.z * (float)timestep, glm::vec3(0.0,0.0,1.0));
                glm::quat finalOrientation = QuatAroundX * QuatAroundY * QuatAroundZ;
                //std::cout << "velocity " << glm::to_string(rigidbody.localMomentOfInertia) << " so we at " << glm::to_string(QuatAroundX) << " \n";
                transform.SetRot(transform.rotation() * finalOrientation);
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
        comp->SetPos(comp->position() + offset);
    }
}
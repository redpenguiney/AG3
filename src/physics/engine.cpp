#include "../gameobjects/component_pool.hpp"
#include "../gameobjects/rigidbody_component.hpp"
#include "spatial_acceleration_structure.hpp"
#include "../gameobjects/component_registry.hpp"
#include <cassert>
#include <cstdio>
#include <memory>
#include <utility>
#include <vector>
#include "engine.hpp"
#include "gjk.hpp"
#include "../../external_headers/GLM/gtx/string_cast.hpp"


PhysicsEngine::PhysicsEngine() {
    //GRAVITY = {0, -0.1, 0};
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
    // TODO: this does mean redundant narrowphase collision checks, we should store results of narrowphase checks to avoid that
    for (auto & otherColliderPtr: potentialColliding) {
        if (otherColliderPtr == &collider) {continue;} // collider shouldn't collide with itself lol
        auto collisionTestResult = IsColliding(transform, collider, *otherColliderPtr->GetGameObject()->transformComponent.ptr, *otherColliderPtr);
        if (collisionTestResult) {
            static unsigned int DEBUG_IGNORE_1;
            std::cout << "it " << DEBUG_IGNORE_1 << "\n";
            if (DEBUG_IGNORE_1 != 0) {
                std::cout << "returning because it " << DEBUG_IGNORE_1 << "\n";
                return;
            }
            DEBUG_IGNORE_1++;
            std::cout << "Object is at " << glm::to_string(transform.position()) << "\n";
            std::cout << "COLLISION!!! normal is " << glm::to_string(collisionTestResult->collisionNormal) << " rel position " << glm::to_string(collisionTestResult->hitPoints.back()) << " distance " << collisionTestResult->penetrationDepth << "\n"; 
            
            auto seperationVector = collisionTestResult->collisionNormal * (otherColliderPtr->GetGameObject()->rigidbodyComponent.ptr ? 0.5 * collisionTestResult->penetrationDepth : collisionTestResult->penetrationDepth);
            {
                static auto m = Mesh::FromFile("../models/rainbowcube.obj", true, true, -1.0, 1.0, 16384);
                GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex});
                params.meshId = m->meshId;
                auto g = ComponentRegistry::NewGameObject(params);
                g->transformComponent->SetPos(collisionTestResult->hitPoints.back() + seperationVector);
                g->transformComponent->SetScl({0.1, 0.1, 0.1});
                g->renderComponent->SetColor({0.5, 0.5, 0.5, 1});
                g->renderComponent->SetTextureZ(-1);
                auto g2 = ComponentRegistry::NewGameObject(params);
                g2->transformComponent->SetPos(transform.position());
                g2->transformComponent->SetScl({0.1, 0.1, 0.1});
                g2->renderComponent->SetColor({0.9, 0.9, 0.5, 1});
                g2->renderComponent->SetTextureZ(-1);
            }
            

            // find center of contact region and apply the torque from there
            glm::dvec3 averageContactPoint = {0, 0, 0}; // in object space
            for (auto & p: collisionTestResult->hitPoints) {
                averageContactPoint += p;
            }
            averageContactPoint /= collisionTestResult->hitPoints.size();

            // velocity of the actual point that hit should be used, not the overall velocity of the object
            glm::vec3 contactPointInObjectSpace = averageContactPoint - transform.position(); // TODO: might need to use whole inverted matrix
            
            glm::dvec3 velocityAtContactPoint = rigidbody.velocity;// + (glm::dvec3)(glm::cross(rigidbody.angularVelocity, contactPointInObjectSpace) * contactPointInObjectSpace);
            //glm::vec3 contactPointInOtherObjectSpace = averageContactPoint - otherColliderPtr->GetGameObject()->transformComponent->position(); // TODO: might need to use whole inverted matrix
            glm::dvec3 velocityAtOtherContactPoint = otherColliderPtr->GetGameObject()->rigidbodyComponent.ptr == nullptr ? glm::dvec3(0, 0, 0) : otherColliderPtr->GetGameObject()->rigidbodyComponent->velocity;
            //glm::dvec3 velocityAtOtherContactPoint = otherColliderPtr->GetGameObject()->rigidbodyComponent.ptr == nullptr ? glm::dvec3(0, 0, 0) : (otherColliderPtr->GetGameObject()->rigidbodyComponent->velocity + (glm::dvec3)(glm::cross(otherColliderPtr->GetGameObject()->rigidbodyComponent->angularVelocity, contactPointInOtherObjectSpace) * contactPointInOtherObjectSpace));

            seperations.emplace_back(std::make_pair(&transform, seperationVector));
            auto elasticity = 0.9;//otherColliderPtr->elasticity * collider.elasticity;
            auto relVelocity = (velocityAtOtherContactPoint - velocityAtContactPoint);
            auto desiredChangeInVelocity = (otherColliderPtr->GetGameObject()->rigidbodyComponent.ptr ? elasticity : 2.0 * elasticity) * collisionTestResult->collisionNormal * (glm::dot(collisionTestResult->collisionNormal, relVelocity) / glm::dot(collisionTestResult->collisionNormal, collisionTestResult->collisionNormal));
            rigidbody.accumulatedForce = desiredChangeInVelocity * (double)rigidbody.mass;

            // torque = cross(radius, force)
            std::cout << "angular velocity is " << glm::to_string(rigidbody.angularVelocity) << " so velocity at point is " << glm::to_string((glm::dvec3)(glm::cross(rigidbody.angularVelocity, contactPointInObjectSpace) * contactPointInObjectSpace)) << ".\n";
            std::cout << " Contact point in object space is " << glm::to_string(contactPointInObjectSpace) << " \n";
            std::cout << " Delta-v is " << glm::to_string(desiredChangeInVelocity) << " \n";
            //rigidbody.accumulatedTorque += glm::cross(((glm::dvec3)contactPointInObjectSpace), desiredChangeInVelocity * (double)rigidbody.mass);
        }
        else {
            //std::cout << "aw.\n";
        }
    }
}

void PhysicsEngine::Step(const double timestep) {

    

    // iterate through all sets of colliderComponent + rigidBodyComponent + transformComponent
    auto components = ComponentRegistry::GetSystemComponents<TransformComponent, SpatialAccelerationStructure::ColliderComponent, RigidbodyComponent>();

    // first pass, convert applied force to velocity and move everything by its velocity
    for (auto & tuple: components) {
        TransformComponent& transform = *std::get<0>(tuple);
        SpatialAccelerationStructure::ColliderComponent& collider = *std::get<1>(tuple);
        RigidbodyComponent& rigidbody = *std::get<2>(tuple);
        if (collider.live) {
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
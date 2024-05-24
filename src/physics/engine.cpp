#include "../gameobjects/component_pool.hpp"
#include "../gameobjects/rigidbody_component.hpp"
#include "GLM/geometric.hpp"
#include "GLM/gtx/string_cast.hpp"
#include "debug/log.hpp"
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

#ifdef IS_MODULE
PhysicsEngine* _PHYSICS_ENGINE_ = nullptr;
void PhysicsEngine::SetModulePhysicsEngine(PhysicsEngine* engine) {
    _PHYSICS_ENGINE_ = engine;
}
#endif

PhysicsEngine& PhysicsEngine::Get() {
    #ifdef IS_MODULE
    assert(_PHYSICS_ENGINE_ != nullptr);
    return *_PHYSICS_ENGINE_;
    #else
    static PhysicsEngine engine; // yeah apparently you can have local static variables
    return engine;
    #endif
}



// Simulates physics of a single rigidbody.
void DoPhysics(const double dt, ColliderComponent& collider, TransformComponent& transform, RigidbodyComponent& rigidbody, std::vector<std::pair<TransformComponent*, glm::dvec3>>& seperations) {    
    // TODO: should REALLY use tight fitting AABB here
    auto aabbToTest = collider.GetAABB();
    std::vector<ColliderComponent*> potentialColliding = SpatialAccelerationStructure::Get().Query(aabbToTest);
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

            // find center of contact region
            glm::dvec3 averageContactPoint = {0, 0, 0}; // in object space
            double averagePenetration = 0;
            for (auto & p: collisionTestResult->contactPoints) {
                // std::cout << "One point is " << glm::to_string(p.first) << ".\n";
                averageContactPoint += p.first;
                averagePenetration += p.second;
                // DebugPlacePointOnPosition({p.first}, {0.5, 0.0, 0.0, 1.0});
            }
            averageContactPoint /= collisionTestResult->contactPoints.size();
            averagePenetration /= collisionTestResult->contactPoints.size();

            glm::vec3 normal = -collisionTestResult->collisionNormal; // normal faces out of the first object towards the other object
            

            // collision impulse-based response stolen from https://physics.stackexchange.com/questions/686640/resolving-angular-components-in-2d-circular-rigid-body-collision-response
            // and this https://physics.stackexchange.com/questions/743172/simulating-rigid-body-collisions-in-3d
            // also this source looks useful https://gafferongames.com/post/collision_response_and_coulomb_friction/
            // and this https://gamedev.stackexchange.com/questions/131219/rigid-body-physics-resolution-causing-never-ending-bouncing-and-jittering?rq=1
            // http://www.chrishecker.com/Rigid_Body_Dynamics
            // https://graphics.stanford.edu/papers/rigid_bodies-sig03/rigid_bodies.pdf
            // this engine implementation seems simple and does similar stuff to us https://github.com/NathanMacLeod/physics3D/blob/master/Physics/PhysicsEngine.cpp#L71

            // seperate colliding objects
            const double LINEAR_SLOP = 0.0;
            auto seperationVector = glm::dvec3(-normal) * ((otherRigidbody ? 0.5 * averagePenetration : averagePenetration) - LINEAR_SLOP);
                // DebugPlacePointOnPosition({transform.Position() + seperationVector}, {1, 0.2, 0.2, 1.0});
            seperations.emplace_back(std::make_pair(&transform, seperationVector));


            DebugPlacePointOnPosition({averageContactPoint}, {0.2, 0.2, 1.0, 1.0});
    
            glm::vec3 d1 = (transform.Position() - averageContactPoint); // contact to pos    
            glm::vec3 d2 = (otherTransform->Position() - averageContactPoint); // contact to otherPos
            glm::vec3 relVelocity = glm::vec3(rigidbody.velocity);// + glm::cross(d1, rigidbody.angularVelocity));
            if (otherRigidbody) {
                relVelocity -= glm::vec3(otherRigidbody->velocity) + glm::cross(d2, otherRigidbody->angularVelocity);
            }
            float relVelocityAlongNormal = glm::dot(normal, relVelocity);
            // collision impulse
            glm::vec3 collisionResolutionImpulse;
            {
                
                glm::vec3 torqueAxis1 = glm::cross(normal, d1); // rCLdeXn ; axis around which torque is applied

                float inverseMomentOfInertiaAroundAxis1 = 0; // if collision angle is directly perpendicular torque doesn't happen, gonna get divide by zero problems
                if (glm::length2(torqueAxis1) >= FLT_EPSILON) {
                    // DebugLogInfo("NORMALIZING ", glm::to_string(torqueAxis1), glm::length2(torqueAxis1));
                    torqueAxis1 = glm::normalize(torqueAxis1);
                    inverseMomentOfInertiaAroundAxis1 = rigidbody.InverseMomentOfInertiaAroundAxis(transform, torqueAxis1);
                }
                
        
                float reducedMass = rigidbody.InverseMass() + glm::length2(torqueAxis1) * inverseMomentOfInertiaAroundAxis1;
                
                if (otherRigidbody) {

                    glm::vec3 torqueAxis2 = glm::cross(normal, d2); // rCLdrXn ; -1 * axis around which torque is applied
                    
                    float inverseMomentOfInertiaAroundAxis2 = 0; // if collision angle is directly perpendicular torque obvi irrelevant
                    if (glm::length2(torqueAxis2) >= FLT_EPSILON) {
                        torqueAxis2= glm::normalize(torqueAxis2);
                        inverseMomentOfInertiaAroundAxis2 = otherRigidbody->InverseMomentOfInertiaAroundAxis(*otherTransform, torqueAxis2);
                    }
                    
                    reducedMass += otherRigidbody->InverseMass() + glm::length2(torqueAxis2) * inverseMomentOfInertiaAroundAxis2;
    
                }
                
                reducedMass = 1.0/reducedMass;

                // float reducedMass = rigidbody.InverseMass() + glm::dot(nxd1, rigidbody.GetInverseGlobalMomentOfInertia(transform) * nxd1);
                // if (otherRigidbody) {
                //     reducedMass += otherRigidbody->InverseMass() + glm::dot(nxd2, rigidbody.GetInverseGlobalMomentOfInertia(*otherTransform) * nxd2);
                // }
                // reducedMass = 1.0 / reducedMass;
                if (reducedMass > rigidbody.InverseMass()) {
                    DebugLogError("Calculated reduced contact mass of ", reducedMass, " (1/", 1.0/reducedMass , "). n = ", glm::to_string(normal), " d1 = ", glm::to_string(d1), " nxd1 = ", glm::to_string(torqueAxis1));
                }

                // assert(reducedMass <= rigidbody.InverseMass());

            
                float elasticity = otherColliderPtr->elasticity * collider.elasticity;
                float elasticityTerm = -(1 + elasticity);
                
                float impulse = elasticityTerm * relVelocityAlongNormal * reducedMass;

                collisionResolutionImpulse = normal * impulse;
                rigidbody.accumulatedForce += collisionResolutionImpulse;
                DebugLogInfo("Torque axis is ", glm::to_string(torqueAxis1), " normal ", glm::to_string(normal), " d1 ", glm::to_string(d1), " crossing those gives ", glm::to_string(glm::cross(normal, d1)));
                rigidbody.accumulatedTorque += -torqueAxis1 * impulse;
            }

            // friction impulse #3: electric boogalee
            {
                glm::vec3 tangentVelocity = relVelocity - (normal * relVelocityAlongNormal);
                if (glm::length2(tangentVelocity) != 0) {
                    // DebugLogInfo("Tangent V = ", glm::to_string(tangentVelocity), ", relV = ", glm::to_string(relVelocity), ", along normal = ", glm::to_string(normal * relVelocityAlongNormal));
                    glm::vec3 tangentDirection = glm::normalize(tangentVelocity);

                    glm::vec3 torqueAxis1 = glm::cross(tangentDirection, d1);
                    float inverseMomentOfInertiaAroundAxis1 = rigidbody.InverseMomentOfInertiaAroundAxis(transform, torqueAxis1);

                    float reducedMass = rigidbody.InverseMass() + glm::length2(torqueAxis1) * inverseMomentOfInertiaAroundAxis1;

                    if (otherRigidbody) {

                    }

                    reducedMass = 1.0/reducedMass;
                    if (reducedMass > rigidbody.InverseMass()) {
                        DebugLogError("Calculated reduced contact mass of ", reducedMass, " (1/", 1.0/reducedMass , "). n = ", glm::to_string(normal), " d1 = ", glm::to_string(d1), " nxd1 = ", glm::to_string(torqueAxis1));
                    }

                    
                    // this is the impulse that would completely halt the objects.
                    float impulse =  glm::dot(tangentDirection, relVelocity) * reducedMass;
                    
                    float friction = otherColliderPtr->friction * collider.friction;
                    float maxPossibleImpulse = glm::length(collisionResolutionImpulse) * friction;
                    if (maxPossibleImpulse < abs(impulse)) {
                        impulse = maxPossibleImpulse;
                    }

                    // rigidbody.accumulatedForce -= tangentDirection * impulse;
                    // rigidbody.accumulatedTorque += torqueAxis1 * impulse;
                }
            }

            // friction impulse
            // {
            //     float friction = otherColliderPtr->friction * collider.friction;
            //     glm::vec3 relVelocityAlongPlane = glm::vec3(glm::vec3(rigidbody.velocity) + glm::cross(d1, rigidbody.angularVelocity)) - (normal * relVelocityAlongNormal);
            //     if (glm::length(relVelocityAlongPlane) != 0) {
            //         glm::vec3 tangent = glm::normalize(relVelocityAlongPlane);

            //         glm::vec3 txd1 = glm::cross(tangent, d1);
            //         // glm::vec3 txd2 = glm::cross(tangent, d2);
        
            //         // float reducedMass = 1.0 / (rigidbody.InverseMass() + glm::dot(txd1, rigidbody.GetInverseGlobalMomentOfInertia(transform) * txd1));
            //         // if (reducedMass > rigidbody.InverseMass()) {
            //         //     DebugLogError("Calculated reduced friction mass of ", reducedMass, " (1/", 1.0/reducedMass , "). t = ", glm::to_string(tangent), " d1 = ", glm::to_string(d1), " txd1 = ", glm::to_string(txd1), " mmoi*txd1 = ", glm::to_string(rigidbody.GetInverseGlobalMomentOfInertia(transform) * txd1), " dotting that yields ", glm::dot(txd1, rigidbody.GetInverseGlobalMomentOfInertia(transform) * txd1));
            //         // }
                    
            //         float frictionImpulse = friction * reducedMass;

            //         // check if friction is strong enough to reverse the object's speed and if so, set speed to 0 instead so we don't start going backwards due to friction
            //         if (glm::length(relVelocityAlongPlane) < frictionImpulse * rigidbody.InverseMass()) {
            //             DebugLogInfo("Friction totally stopped the object.");
            //             rigidbody.accumulatedForce += -relVelocityAlongPlane / rigidbody.InverseMass();
            //         }
            //         else {
            //             DebugLogInfo("Applying force with tangent ", glm::to_string(tangent), " and friction impulse ", frictionImpulse, ". Rel. tangent velocity was ", glm::to_string(relVelocityAlongPlane), " torque axis ", glm::to_string(glm::cross(d1, tangent)));
            //             rigidbody.accumulatedForce -= tangent * frictionImpulse;
            //             rigidbody.accumulatedTorque -= glm::cross(normal, tangent) * frictionImpulse;
            //         }

                    
            //     }
                
                
            // // }
            // // std::cout << "Accumulated force is now " << glm::to_string(rigidbody.accumulatedForce) << ". Impulse is " << impulse << ". Reduced mass is " << reducedMass << ".\n";
            // }

            // FRICTION
            // glm::vec3 relVelocityAlongPlane =  relVelocity + (normal * relVelocityAlongContactNormal);
            // if (glm::length(relVelocityAlongPlane) > 0) {
            //     auto tangentDirection = glm::normalize(relVelocityAlongPlane);

            //     glm::vec3 planarCrossThing1 = glm::cross((glm::vec3)tangentDirection, contactToRigidbody);
            //     glm::vec3 planarCrossThing2 = glm::cross((glm::vec3)tangentDirection, contactToOtherRigidbody);
            //     // std::cout << "Along plane, rel velocity is " << glm::to_string(relVelocityAlongPlane) << " as calculated from total rel vel " << glm::to_string(relVelocity) << " and vel along normal " << relVelocityAlongContactNormal << ".\n";
                
            //     
            //     // double reducedInverseMassAlongPlane = 1/(rigidbody.InverseMass() + (otherRigidbody ? otherRigidbody->InverseMass() : 0) + glm::dot(planarCrossThing1, (globalInverseInertiaTensor1 * planarCrossThing1)) + glm::dot(planarCrossThing2, otherRigidbody ? (globalInverseInertiaTensor2 * planarCrossThing2): glm::vec3(0, 0, 0)));
            //     // float frictionImpulse = friction * reducedInverseMassAlongPlane * glm::length(relVelocityAlongPlane);

            //     // if (glm::length(relVelocityAlongPlane) > friction * )
            //     // rigidbody.accumulatedForce += frictionImpulse * tangentDirection;   
            //     // rigidbody.accumulatedTorque -= glm::cross(-posRelToContact, -tangentDirection) * frictionImpulse;
            // }

        }
        else {
            //std::cout << "aw.\n";
        }
    }
}

void PhysicsEngine::Step(const double timestep) {

    

    // iterate through all sets of colliderComponent + rigidBodyComponent + transformComponent
    auto components = ComponentRegistry::Get().GetSystemComponents<TransformComponent, ColliderComponent, RigidbodyComponent>();

    // first pass, apply gravity, convert applied force to velocity, apply drag, and move everything by its velocity
    for (auto & tuple: components) {
        TransformComponent& transform = *std::get<0>(tuple);
        ColliderComponent& collider = *std::get<1>(tuple);
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
            rigidbody.angularVelocity += glm::vec3(rigidbody.InverseMomentOfInertiaAroundAxis(transform, {1, 0, 0}), rigidbody.InverseMomentOfInertiaAroundAxis(transform, {0, 1, 0}), rigidbody.InverseMomentOfInertiaAroundAxis(transform, {0, 0, 1})) * rigidbody.accumulatedTorque;
            rigidbody.accumulatedTorque = {0, 0, 0};

            if (rigidbody.velocity != glm::dvec3(0, 0, 0)) {
                transform.SetPos(transform.Position() + rigidbody.velocity * timestep);
            }
           
            if (rigidbody.angularVelocity != glm::vec3(0, 0, 0)) {
                glm::quat spin = glm::quat(0, rigidbody.angularVelocity.x, rigidbody.angularVelocity.y, rigidbody.angularVelocity.z) * 0.5f * float(timestep);
                //std::cout << "velocity " << glm::to_string(rigidbody.localMomentOfInertia) << " so we at " << glm::to_string(QuatAroundX) << " \n";
                assert(!std::isnan(spin.x));
                transform.SetRot(glm::normalize(transform.Rotation() + spin));
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
        ColliderComponent& collider = *std::get<1>(tuple);
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
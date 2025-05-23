#include "rigidbody_component.hpp"
#include "glm/ext.hpp"
#include "glm/gtx/string_cast.hpp"
#include "debug/log.hpp"
#include "gameobjects/rigidbody_component.hpp"
#include "gameobjects/transform_component.hpp"
#include "physics/physics_mesh.hpp"
#include "debug/assert.hpp"
#include <iostream>
#include <limits>

RigidbodyComponent::RigidbodyComponent(const std::shared_ptr<PhysicsMesh>& physMesh): physicsMesh(physMesh) {
    Assert(physMesh != nullptr);

    kinematic = false;
    velocity = {0, 0, 0};
    accumulatedForce = {0, 0, 0};
    angularVelocity = {0, 0, 0}; 
    accumulatedTorque = {0, 0, 0};
    linearDrag = 0.99;
    angularDrag = 0.999;
    
    MakeMassInfinite();
}

RigidbodyComponent::~RigidbodyComponent() {

}

//void RigidbodyComponent::Destroy() {
//    physicsMesh = nullptr;
//}

float RigidbodyComponent::InverseMass() const {
    return inverseMass;
}

void RigidbodyComponent::SetMass(float newMass, const TransformComponent& transform) {
    Assert(newMass != 0);
    inverseMass = 1.0f/newMass;
    UpdateMomentOfInertia(transform);
}

void RigidbodyComponent::UpdateMomentOfInertia(const TransformComponent& transform) {
    // DebugLogInfo("Oh? ", physicsMesh);
    if (inverseMass == 0.0f) {
        localMomentOfInertia = glm::mat3x3(INFINITY);
    }
    else {
        // DebugLogInfo("calculating");
        localMomentOfInertia = physicsMesh->CalculateLocalMomentOfInertia(transform.Scale(), 1.0f/inverseMass);
    }
    
}

void RigidbodyComponent::MakeMassInfinite() { // TODO: untested.
    inverseMass = 0.0f;
    localMomentOfInertia = glm::mat3x3(INFINITY);
}

// mainly so that division by zero gives infinity not undefined behavior
static_assert(std::numeric_limits<double>::is_iec559, "Physics engine expects IEEE floating point compliance.");
static_assert(std::numeric_limits<float>::is_iec559, "Physics engine expects IEEE floating point compliance.");

float RigidbodyComponent::InverseMomentOfInertiaAroundAxis(const TransformComponent& transform, glm::vec3 axis) const {
    auto length2 = glm::length2(axis);
    Assert(length2 != 0);
    // DebugLogInfo("Rot is ", glm::to_string(transform.Rotation()), " inverse rot is ", glm::to_string(glm::inverse(transform.Rotation())));
    glm::vec3 axisInLocalSpace = glm::inverse(transform.Rotation()) * axis;
    glm::vec3 inertiaAxis = localMomentOfInertia * axisInLocalSpace;

    //TODO: this makes me sad performance-wise
    //if the inertia on an axis is infinite but the local-space axis is 0, 0*inf is nan which is no good so we just make it 0 which produces expected result. 
    for (unsigned int i = 0; i < glm::vec3::length(); i++) {
        if (std::isnan(inertiaAxis[i])) {
            inertiaAxis[i] = 0;
        }
    }

    float dot = glm::dot(axisInLocalSpace, inertiaAxis);
    float inverseMoment = 1.0 / dot;

    //DebugLogInfo("\tReturning dot ", dot, ", 1.0/that is ", inverseMoment, " from i-axis ", glm::to_string(inertiaAxis), " local ", glm::to_string(axisInLocalSpace), " localmomomiooii = ", glm::to_string(localMomentOfInertia));

    Assert(!std::isnan(inverseMoment));
	return inverseMoment;
}

// fyi position is in world space minus the position of the rigidbody (so not model space since it does rotation/scaling)
// don't expect velocity to change until physics engine steps
void RigidbodyComponent::Impulse(glm::vec3 position, glm::vec3 force) {
    
    Assert(!std::isnan(glm::length2(position)));
    Assert(!std::isnan(glm::length2(force)));
    

    // thanks to madeleine for her assistance with physics
    // you thought f equaled ma, didn't you you? WRONG, it's not that shrimple

    // std::cout << "Applying force " << glm::to_string(force) << " at " << glm::to_string(position) << ".\n";

    // float positionToXAxis = glm::length(glm::vec2(position.y, position.z));
    // float positionToYAxis = glm::length(glm::vec2(position.x, position.z));
    // float positionToZAxis = glm::length(glm::vec2(position.x, position.y));
    // std::cout << "Axis distances are " << glm::to_string(glm::abs(glm::vec3(positionToXAxis, positionToYAxis, positionToZAxis))) << ".\n";
    accumulatedTorque += glm::cross(position, force);  //glm::abs(glm::vec3(positionToXAxis, positionToYAxis, positionToZAxis)) * force;
    // std::cout << "Torque is " << glm::to_string(accumulatedTorque) << ".\n";
    accumulatedForce += force;

}

// fyi position is in world space minus the position of the rigidbody (so not model space since it does rotation/scaling)
// almost certainly correctly implemented at this point
// velocity is relative to center of mass (add overall object velocity to get world velocity)
glm::dvec3 RigidbodyComponent::VelocityAtPoint(glm::vec3 position) {
    

    // float positionToXAxis = glm::length(glm::vec2(position.y, position.z));
    // float positionToYAxis = glm::length(glm::vec2(position.x, position.z));
    // float positionToZAxis = glm::length(glm::vec2(position.x, position.y));
    
    //std::cout << "Rigidbody has angular velocity " << glm::to_string(angularVelocity) << " so at position " << glm::to_string(position) << " it has linear velocity " << glm::to_string(glm::cross(position, angularVelocity)) << ".\n";
    return glm::cross(position, glm::degrees(angularVelocity));
}
//RigidbodyComponent::RigidbodyComponent() {
//    physicsMesh = nullptr;
//}
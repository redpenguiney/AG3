#include "rigidbody_component.hpp"
#include "../../external_headers/GLM/ext.hpp"
#include "../../external_headers/GLM/gtx/string_cast.hpp"
#include <iostream>

void RigidbodyComponent::Init() {
    kinematic = false;
    velocity = {0, 0, 0};
    accumulatedForce = {0, 0, 0};
    angularVelocity = {0, 0, 0}; 
    accumulatedTorque = {0, 0, 0};
    momentOfInertia = {1.0/3.0, 1.0/3.0, 1.0/3.0}; // TODO: that's the moment for a cube with size 1x1x1 and mass 1
    linearDrag = 0.999;
    angularDrag = 0.99;
    mass = 1;
    
}

void RigidbodyComponent::Destroy() {
    
}

// fyi position is in world space minus the position of the rigidbody (so not model space since it does rotation/scaling)
// don't expect velocity to change until physics engine steps
void RigidbodyComponent::Impulse(glm::vec3 position, glm::vec3 force) {
    
    // thanks to madeleine for her assistance here and elsewhere with physics
    // you thought f equaled ma, didn't you you? WRONG

    std::cout << "Applying force " << glm::to_string(force) << " at " << glm::to_string(position) << ".\n";

    // float positionToXAxis = glm::length(glm::vec2(position.y, position.z));
    // float positionToYAxis = glm::length(glm::vec2(position.x, position.z));
    // float positionToZAxis = glm::length(glm::vec2(position.x, position.y));
    // std::cout << "Axis distances are " << glm::to_string(glm::abs(glm::vec3(positionToXAxis, positionToYAxis, positionToZAxis))) << ".\n";
    // accumulatedTorque += glm::cross(position, force);  //glm::abs(glm::vec3(positionToXAxis, positionToYAxis, positionToZAxis)) * force;
    std::cout << "Torque is " << glm::to_string(accumulatedTorque) << ".\n";
    accumulatedForce += force;
}

// fyi position is in world space minus the position of the rigidbody (so not model space since it does rotation/scaling)
// almost certainly correctly implemented at this point
// velocity is relative to center of mass (add overall object velocity to get world velocity)
glm::dvec3 RigidbodyComponent::VelocityAtPoint(glm::vec3 position) {
    

    float positionToXAxis = glm::length(glm::vec2(position.y, position.z));
    float positionToYAxis = glm::length(glm::vec2(position.x, position.z));
    float positionToZAxis = glm::length(glm::vec2(position.x, position.y));
    
    std::cout << "Rigidbody has angular velocity " << glm::to_string(angularVelocity) << " so at position " << glm::to_string(position) << " it has linear velocity " << glm::to_string(glm::cross(angularVelocity, glm::vec3(positionToXAxis, positionToYAxis, positionToZAxis))) << ".\n";
    return glm::cross(angularVelocity, glm::vec3(positionToXAxis, positionToYAxis, positionToZAxis));
}
RigidbodyComponent::RigidbodyComponent() {}
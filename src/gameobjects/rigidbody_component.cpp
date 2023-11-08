#include "rigidbody_component.hpp"

void RigidbodyComponent::Init() {
    kinematic = false;
    velocity = {0, 0, 0};
    accumulatedForce = {0, 0, 0};
    angularVelocity = {0, 0, 0};
    mass = 1;
}

void RigidbodyComponent::Destroy() {
    
}
#include "rigidbody_component.hpp"

void RigidbodyComponent::Init() {
    kinematic = false;
    velocity = {0, 0, 0};
    accumulatedForce = {0, 0, 0};
    angularVelocity = {0, 0, 0};
    accumulatedTorque = {0, 0, 0};
    momentOfInertia = {2/3, 2/3, 2/3}; // TODO: that's the moment for a sphere with radius 1 mass 1
    mass = 1;
}

void RigidbodyComponent::Destroy() {
    
}

RigidbodyComponent::RigidbodyComponent() {}
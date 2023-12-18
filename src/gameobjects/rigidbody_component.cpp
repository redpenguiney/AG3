#include "rigidbody_component.hpp"

void RigidbodyComponent::Init() {
    kinematic = false;
    velocity = {0, 0, 0};
    accumulatedForce = {0, 0, 0};
    angularVelocity = {0, 0, 0}; // around x, y, and z axis, in radians/second
    accumulatedTorque = {0, 0, 0};
    momentOfInertia = {1.0/3.0, 1.0/3.0, 1.0/3.0}; // TODO: that's the moment for a cube with size 1x1x1 and mass 1
    mass = 1;
    
}

void RigidbodyComponent::Destroy() {
    
}

RigidbodyComponent::RigidbodyComponent() {}
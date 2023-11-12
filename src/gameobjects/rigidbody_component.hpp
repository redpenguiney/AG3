#pragma once
#include "base_component.hpp"
#include "../../external_headers/GLM/vec3.hpp"

class RigidbodyComponent: public BaseComponent<RigidbodyComponent> {
    public:
    bool kinematic; // if true, the physics engine will do nothing to this component except change its position/rotation by its velocity

    glm::dvec3 velocity;
    glm::vec3 accumulatedForce; // every frame, accumulated forces are turned into velocity (f=ma) and then set to zero
    glm::vec3 angularVelocity; // not a quaternion since those wouldn't be able to represent a rotation >360 degrees every frame

    float mass; // may not be zero, probably shouldn't be negative

    void Init();
    void Destroy();
};
#pragma once
#include "base_component.hpp"
#include "../../external_headers/GLM/vec3.hpp"

class RigidbodyComponent: public BaseComponent<RigidbodyComponent> {
    public:
    bool kinematic; // if true, the physics engine will do nothing to this component except change its position/rotation by its velocity

    glm::dvec3 velocity;
    glm::vec3 accumulatedForce; // every physics timestep, accumulated forces are turned into change in velocity (f=ma) and then set to zero

    glm::vec3 angularVelocity; // not a quaternion since those wouldn't be able to represent a rotation >360 degrees every frame
                               // around x, y, and z axis, in radians/second
    glm::vec3 accumulatedTorque; // like accumulateForce, but for torque (rotational force), converted to change in angular velocity via torque/momentOfInertia
    glm::vec3 momentOfInertia; // sorta like mass but for rotation; how hard it is to rotate something around each axis basically

    double linearDrag; // rigid body's velocity will be multiplied by this every frame
    float angularDrag; // rigid body's angular velocity will be multiplied by this every frame

    float mass; // may not be zero, probably shouldn't be negative

    void Init();
    void Destroy();

    // Applies the given force at the given position on the object, applying the correct amount of torque.
    // position is in world space minus the position of the rigidbody (so not model space since it does rotation/scaling)
    void Impulse(glm::vec3 position, glm::vec3 force);

    // Returns the velocity of the given point on the object RELATIVE TO the object's centre. (so add rigidbody.velocity to get full velocity)
    // (effectively converts the rigidbody's angular velocity to linear)
    // position is in world space minus the position of the rigidbody (so not model space since it does rotation/scaling)
    glm::dvec3 VelocityAtPoint(glm::vec3 position);

    private:
    //private constructor to enforce usage of object pool
    friend class ComponentPool<RigidbodyComponent>;
    RigidbodyComponent();
    RigidbodyComponent(const RigidbodyComponent& other) = delete;
};
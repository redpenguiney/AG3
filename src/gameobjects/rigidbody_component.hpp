#pragma once
#include "base_component.hpp"
#include "transform_component.hpp"
#include "../../external_headers/GLM/vec3.hpp"
#include "../../external_headers/GLM/mat3x3.hpp"

// TODO: RIGIDBODIES CURRENTLY DON'T DO PHYSICS IF THEY DON'T HAVE A COLLIDER, MB
class RigidbodyComponent: public BaseComponent<RigidbodyComponent> {
    public:
    bool kinematic; // if true, the physics engine will do nothing to this component except change its position/rotation by its velocity

    glm::dvec3 velocity;
    glm::vec3 accumulatedForce; // every physics timestep, accumulated forces are turned into change in velocity (f=ma) and then set to zero

    glm::vec3 angularVelocity; // not a quaternion since those wouldn't be able to represent a rotation >360 degrees every frame
                               // around x, y, and z axis, in radians/second
                               // axis are in world space?
    glm::vec3 accumulatedTorque; // like accumulateForce, but for torque (rotational force), converted to change in angular velocity via torque/globalMomentOfInertia
    glm::mat3x3 localMomentOfInertia; // sorta like mass but for rotation; how hard it is to rotate something around each axis basically; must be converted to global space

    double linearDrag; // rigid body's velocity will be multiplied by this every frame
    float angularDrag; // rigid body's angular velocity will be multiplied by this every frame

    // returns 1/mass of the rigid body.
    float InverseMass() const;

    // newMass must not be zero
    void SetMass(float newMass);

    void Init();
    void Destroy();

    

    // Applies the given force at the given position on the object, applying the correct amount of torque.
    // position is in world space minus the position of the rigidbody (so not model space since it does rotation/scaling)
    void Impulse(glm::vec3 position, glm::vec3 force);

    // Returns the velocity of the given point on the object RELATIVE TO the object's centre. (so add rigidbody.velocity to get full velocity)
    // (effectively converts the rigidbody's angular velocity to linear)
    // position is in world space minus the position of the rigidbody (so not model space since it does rotation/scaling)
    glm::dvec3 VelocityAtPoint(glm::vec3 position);

    // returns inverse moment of inertia in world space, given the transform corresponding to the rigidbody
    glm::mat3x3 GetInverseGlobalMomentOfInertia(const TransformComponent& transform);

    private:
    //private constructor to enforce usage of object pool
    friend class ComponentPool<RigidbodyComponent>;
    RigidbodyComponent();
    RigidbodyComponent(const RigidbodyComponent& other) = delete;

    float inverseMass; // we store 1/mass instead of mass because all the formulas use inverse mass and this saves us some division
};
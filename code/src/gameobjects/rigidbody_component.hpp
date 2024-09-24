#pragma once
#include "base_component.hpp"
#include "transform_component.hpp"
#include <glm/vec3.hpp>
#include "glm/mat3x3.hpp"
#include <memory>

class PhysicsMesh;

// Created rigidbodies are immobile (infinite mass & moi) until you call SetMass() on them.
class RigidbodyComponent: public BaseComponent {
    public:
    RigidbodyComponent(const RigidbodyComponent& other) = delete;
    RigidbodyComponent(const std::shared_ptr<PhysicsMesh>& physMesh);
    ~RigidbodyComponent();

    bool kinematic; // if true, the physics engine will do nothing to this component except change its position/rotation by its velocity

    glm::dvec3 velocity;
    glm::vec3 accumulatedForce; // every physics timestep, accumulated forces are turned into change in velocity (f=ma) and then set to zero

    glm::vec3 angularVelocity; // not a quaternion since those wouldn't be able to represent a rotation >360 degrees every frame
                               // around x, y, and z axis, in radians/second
                               // axis are in world space?
    glm::vec3 accumulatedTorque; // like accumulateForce, but for torque (rotational force), converted to change in angular velocity via torque/globalMomentOfInertia
    
    // sorta like mass but for rotation; how hard it is to rotate something around each axis basically; must be converted from object to global space before being used for physics via InverseMomentOfInertiaAroundAxis().
    // You can change this as you please (to achieve effects like making the player's rigidbody not rotate, but note that SetMass() will reset this.
    glm::mat3x3 localMomentOfInertia; 

    double linearDrag; // rigid body's velocity will be multiplied by this every frame
    float angularDrag; // rigid body's angular velocity will be multiplied by this every frame

    // returns 1/mass of the rigid body.
    float InverseMass() const;

    // newMass must not be zero. Sets object mass and localMomentOfInertia. We need access to the transform component to update the moment of inertia along with mass.
    void SetMass(float newMass, const TransformComponent& transform);

    // makes the object impossible to move or rotate. Just a more performant way of passing infinity to SetMass().
    void MakeMassInfinite();

    // You MUST call this if the transform component's scale changes to have correct physics results. TODO: kinda mid to have to do this
    void UpdateMomentOfInertia(const TransformComponent& transform);

    //void Init(const std::shared_ptr<PhysicsMesh>& physMesh);
    //void Destroy();

    

    // Applies the given force at the given position on the object, applying the correct amount of torque.
    // position is in world space minus the position of the rigidbody (so not model space since it does rotation/scaling)
    void Impulse(glm::vec3 position, glm::vec3 force);

    // Returns the velocity of the given point on the object RELATIVE TO the object's centre. (so add rigidbody.velocity to get full velocity)
    // (effectively converts the rigidbody's angular velocity to linear)
    // position is in world space minus the position of the rigidbody (so not model space since it does rotation/scaling)
    glm::dvec3 VelocityAtPoint(glm::vec3 position);

    // returns inverse moment of inertia in world space, given the transform corresponding to the rigidbody
    // glm::mat3x3 GetInverseGlobalMomentOfInertia(const TransformComponent& transform);

    // returns inverse mmoi of the rigidbody around the given axis. Axis is in world space.
    float InverseMomentOfInertiaAroundAxis(const TransformComponent& transform, glm::vec3 axis) const;

    

    private:
    //private constructor to enforce usage of object pool
    //friend class ComponentPool<RigidbodyComponent>;
    

    float inverseMass; // we store 1/mass instead of mass because all the formulas use inverse mass and this saves us some division

    const std::shared_ptr<PhysicsMesh> physicsMesh;
};
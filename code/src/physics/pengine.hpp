#pragma once
#include <glm/vec3.hpp>
#include <vector>
#include "aabb.hpp"
#include <functional>
#include <memory>
#include "vec6.hpp"

struct Position {
    glm::dvec3 pos;
    glm::quat rot;

    vec6 operator-(const Position& other) {
        vec6 result;
        result[0] = pos.x - other.pos.x;
        result[1] = pos.y - other.pos.y;
        result[2] = pos.z - other.pos.z;
        glm::quat quatDif = 2.0f * rot * glm::inverse(other.rot);
        result[3] = quatDif.x;
        result[4] = quatDif.y;
        result[5] = quatDif.z;
        return result;
    }
};

struct Collision {
    CollisionInfo info;
    ColliderComponent* a;
    ColliderComponent* b;
};

class Constraint {
public:
    //    double lagrange;    
    double stiffness;

    //    const double targetStiffness;
    //    const double minLagrange;
    const double maxStiffness;
    //    const bool hard;

    // Returns how badly the constraint is unsatsified 
    virtual std::tuple<double, glm::dvec3, glm::quat> Error(const RigidbodyComponent& a, const RigidbodyComponent& b) = 0;
};

class Contact : public Constraint {
public:
    glm::dvec3 r1;
    glm::dvec3 r2;

    Contact(const Collision&);

    std::tuple<double, glm::dvec3, glm::quat> Error(const RigidbodyComponent& a, const RigidbodyComponent& b) override;
};

// it's a physics engine, obviously.
class PhysicsEngine {
public:

    PhysicsEngine(PhysicsEngine const&) = delete; // no copying
    PhysicsEngine& operator=(PhysicsEngine const&) = delete; // no assigning

    glm::dvec3 GRAVITY;

    // returns pointer to collisionLayerMatrix.
    const std::array<std::bitset<MAX_COLLISION_LAYERS>, MAX_COLLISION_LAYERS>& GetCollisionLayerMatrix();

    // sets whether the given layers collide with each other
    void SetCollisionLayers(CollisionLayer layer1, CollisionLayer layer2, bool collide);

    // float is dt, unlike graphical events dt should be constant. Called in main.cpp because Step() is called repeatedly.
    std::shared_ptr<Event<float>> prePhysicsEvent;
    // float is dt, unlike graphical events dt should be constant. Callded in main.cpp because Step() is called repeatedly.
    std::shared_ptr<Event<float>> postPhysicsEvent;
    
    static PhysicsEngine& Get();

    // When modules (shared libraries) get their copy of this code, they need to use a special version of PhysicsEngine::Get().
    // This is so that both the module and the main executable have access to the same singleton. 
    // The executable will provide each shared_library with a pointer to the physics engine.
    #ifdef IS_MODULE
    static void SetModulePhysicsEngine(PhysicsEngine* engine);
    #endif

    // Moves the physics simulation forward by timestep.
    void Step(const double timestep);

private:

    //std::vector<Constraint> currentConstraints;

    //constexpr static double regularization = 0.95;
    //constexpr static double stiffnessScaling = 0.99;
    constexpr static double stiffnessRamping = 10.0;

    // describes which collision layers interact with each other (true if they collide).
    // defaults to all true.
    std::array<std::bitset<MAX_COLLISION_LAYERS>, MAX_COLLISION_LAYERS> collisionLayerMatrix;

    PhysicsEngine();

    ~PhysicsEngine();
};
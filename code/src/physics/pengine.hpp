#include <glm/vec3.hpp>
#include <vector>
#include "aabb.hpp"

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

    // describes which collision layers interact with each other (true if they collide).
    // defaults to all true.
    std::array<std::bitset<MAX_COLLISION_LAYERS>, MAX_COLLISION_LAYERS> collisionLayerMatrix;

    PhysicsEngine();

    ~PhysicsEngine();
};
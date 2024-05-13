#include "../../external_headers/GLM/vec3.hpp"
#include <vector>

// it's a physics engine, obviously.
class PhysicsEngine {
    public:
    PhysicsEngine(PhysicsEngine const&) = delete; // no copying
    PhysicsEngine& operator=(PhysicsEngine const&) = delete; // no assigning
    glm::dvec3 GRAVITY;
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

    PhysicsEngine();

    ~PhysicsEngine();
};
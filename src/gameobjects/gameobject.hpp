#include <memory>
#include<unordered_set>
#include <vector>
#include "../graphics/engine.hpp"
#include "transform_component.cpp"
#include "../physics/spatial_acceleration_structure.hpp"

class GameObject;
inline std::unordered_map<GameObject*, std::shared_ptr<GameObject>> GAMEOBJECTS;

// TODO
struct CreateGameobjectParams;

// The gameobject system uses ECS (google it).
class GameObject {
    public:

    // all gameobjects will reserve a render, transform, and collision component even if they don't need them to ensure cache stuff
        // TODO: this might not be neccesary
    // GraphicsEngine and SpatialAccelerationStructure rely on this behavior, DO NOT mess with it
    // fyi the const in this position means that the address the pointer points to won't change, not that the pointer points to constant data
    GraphicsEngine::RenderComponent* const renderComponent;
    TransformComponent* const transformComponent;
    SpatialAccelerationStructure::ColliderComponent* const colliderComponent;

    static std::shared_ptr<GameObject> New(unsigned int meshId, unsigned int textureId, bool haveCollisions = true, bool havePhysics = false);

    // NOTE: only removes shared_ptr from GAMEOBJECTS, destructor will not be called until all other shared_ptrs to this gameobject are deleted.
        // Those shared_ptrs remain completely valid and can be read/written freely (although why would you if you're destroying it???).
    // TODO: when Destroy() is called, should still make it stop being drawn and stop physics.
    void Destroy();

    static void Cleanup();

    ~GameObject();

    private:
        // todo: seems mid that this exists
        bool deleted;

        // no copy constructing gameobjects.
        GameObject(const GameObject&) = delete; 

        GameObject(unsigned int meshId, unsigned int textureId, bool haveCollider, bool havePhysics);
        
};
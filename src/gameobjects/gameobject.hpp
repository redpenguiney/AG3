#pragma once
#include <memory>
#include<unordered_set>
#include <vector>
#include "../graphics/engine.hpp"
#include "transform_component.cpp"
#include "../physics/spatial_acceleration_structure.hpp"
#include "pointlight_component.hpp"
#include <optional>

class GameObject;
inline std::unordered_map<GameObject*, std::shared_ptr<GameObject>> GAMEOBJECTS;

struct CreateGameObjectParams {
    bool haveGraphics;
    unsigned int meshId;
    unsigned int textureId;
    unsigned int shaderId;
    
    bool haveCollisions;
    bool havePhysics;

    bool havePointLight;

    CreateGameObjectParams():
    haveGraphics(false),
    meshId(0),
    textureId(0),
    shaderId(0),

    haveCollisions(false),
    havePhysics(false),

    havePointLight(false) {}
};

// The gameobject system uses ECS (google it).
class GameObject {
    public:
    std::string name; // just an identifier i have mainly for debug reasons, scripts could also use it i guess

    // all gameobjects will reserve a render, transform, and collision component even if they don't need them to ensure cache stuff
        // TODO: this might not be neccesary
    // GraphicsEngine and SpatialAccelerationStructure rely on this behavior, DO NOT mess with it
    // fyi the const in this position means that the address the pointer points to won't change, not that the pointer points to constant data
    GraphicsEngine::RenderComponent* const renderComponent;
    TransformComponent* const transformComponent;
    SpatialAccelerationStructure::ColliderComponent* const colliderComponent;

    PointLightComponent* const pointLightComponent;

    static std::shared_ptr<GameObject> New(const CreateGameObjectParams& params);

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

        GameObject(const CreateGameObjectParams& params);
        
};
#include "component_registry.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <vector>
#include "debug/log.hpp"
#include "rigidbody_component.hpp"

// some wacky function i copypasted from https://stackoverflow.com/questions/8147027/how-do-i-call-stdmake-shared-on-a-class-with-only-protected-or-private-const .
// Lets me put use std::make_shared on stuff with private constructors
template < typename Object, typename... Args >
inline std::shared_ptr< Object >
protected_make_shared( Args&&... args )
{
  struct helper : public Object
  {
    helper( Args&&... args )
      : Object{ std::forward< Args >( args )... }
    {}
  };

  return std::make_shared< helper >( std::forward< Args >( args )... );
}

// template<typename ... PoolClasses>


std::shared_ptr<GameObject> ComponentRegistry::NewGameObject(const GameobjectCreateParams& params) {
    if (params.requestedComponents[RenderComponentBitIndex]) { 
        assert(!params.requestedComponents[RenderComponentNoFOBitIndex]); // Can't have both kinds of render components.
    }

    // make sure there are the needed component pools for this kind of gameobject
    if (!componentBuckets.count(params.requestedComponents)) {
        componentBuckets[params.requestedComponents] = std::array<void*, N_COMPONENT_TYPES> {{
            [TransformComponentBitIndex] = params.requestedComponents[TransformComponentBitIndex] ? new ComponentPool<TransformComponent>() : nullptr,
            [RenderComponentBitIndex] = params.requestedComponents[RenderComponentBitIndex] ? new ComponentPool<GraphicsEngine::RenderComponent>() : nullptr,
            [ColliderComponentBitIndex] = params.requestedComponents[ColliderComponentBitIndex] ? new ComponentPool<SpatialAccelerationStructure::ColliderComponent>() : nullptr,
            [RigidbodyComponentBitIndex] = params.requestedComponents[RigidbodyComponentBitIndex] ? new ComponentPool<RigidbodyComponent>() : nullptr,
            [PointlightComponentBitIndex] = params.requestedComponents[PointlightComponentBitIndex] ? new ComponentPool<PointLightComponent>() : nullptr,
            [RenderComponentNoFOBitIndex] = params.requestedComponents[RenderComponentNoFOBitIndex] ? new ComponentPool<GraphicsEngine::RenderComponentNoFO>() : nullptr,
            [AudioPlayerComponentBitIndex] = params.requestedComponents[AudioPlayerComponentBitIndex] ? new ComponentPool<AudioPlayerComponent>() : nullptr
        }};
        //std::cout << "RENDER COMP POOL PAGE AT " << ((ComponentPool<GraphicsEngine::RenderComponent>*)(componentBuckets[params.requestedComponents][RenderComponentBitIndex]))->pools[0] << "\n";
    }

    // Get components for the gameobject
    std::array<void*, N_COMPONENT_TYPES> components;
    for (unsigned int i = 0; i < params.requestedComponents.size(); i++) {
        if (params.requestedComponents[i] == true) { // if the gameobject wants this component
            switch (i) {
            case TransformComponentBitIndex:
            components[i] = ((ComponentPool<TransformComponent>*)(componentBuckets.at(params.requestedComponents).at(i)))->GetNew();
            break;
            case RenderComponentBitIndex:
            components[i] = ((ComponentPool<GraphicsEngine::RenderComponent>*)(componentBuckets.at(params.requestedComponents).at(i)))->GetNew();
            break;
            case RenderComponentNoFOBitIndex:
            components[i] = ((ComponentPool<GraphicsEngine::RenderComponentNoFO>*)(componentBuckets.at(params.requestedComponents).at(i)))->GetNew();
            break;
            case ColliderComponentBitIndex:
            components[i] = ((ComponentPool<SpatialAccelerationStructure::ColliderComponent>*)(componentBuckets.at(params.requestedComponents).at(i)))->GetNew();
            break;
            case RigidbodyComponentBitIndex:
            components[i] = ((ComponentPool<RigidbodyComponent>*)(componentBuckets.at(params.requestedComponents).at(i)))->GetNew();
            break;
            case PointlightComponentBitIndex:
            components[i] = ((ComponentPool<PointLightComponent>*)(componentBuckets.at(params.requestedComponents).at(i)))->GetNew();
            break;
            case AudioPlayerComponentBitIndex:
            components[i] = ((ComponentPool<AudioPlayerComponent>*)(componentBuckets.at(params.requestedComponents).at(i)))->GetNew();
            break;
            default:
            std::printf("you goofy goober you didn't make a case here for index %u\n", i);
            abort();
            break;
            }
        }
        else {
            components[i] = nullptr;
        }
    }

    auto ptr = protected_make_shared<GameObject>(params, components);
    GAMEOBJECTS[ptr.get()] = ptr;
    return ptr;

}

ComponentRegistry::ComponentRegistry() {

}

ComponentRegistry::~ComponentRegistry() {
    DebugLogInfo("Cleaning up all gameobjects.");
    GAMEOBJECTS.clear(); // no way its this simple
    DebugLogInfo("Cleaned up all gameobjects.");
}

#ifdef IS_MODULE
ComponentRegistry* _COMPONENT_REGISTRY_ = nullptr;
void ComponentRegistry::SetModuleComponentRegistry(ComponentRegistry* reg) {
    _COMPONENT_REGISTRY_ = reg;
}
#endif

ComponentRegistry& ComponentRegistry::Get() {
    #ifdef IS_MODULE
    assert(_COMPONENT_REGISTRY_ != nullptr);
    return *_COMPONENT_REGISTRY_;
    #else
    static ComponentRegistry registry;
    return registry;
    #endif
}

void GameObject::Destroy() {
    // DebugLogInfo("Destroy was called on ", name, " (", this, ").");
    if (!ComponentRegistry::Get().GAMEOBJECTS.contains(this)) {
        std::cout << "Error: Destroy() was called on the same gameobject twice, or this gameobject is otherwise invalid. Please don't.\n";
        abort();
    }
    ComponentRegistry::Get().GAMEOBJECTS.erase(this);
    transformComponent.Clear();
    renderComponent.Clear();
    colliderComponent.Clear();
    rigidbodyComponent.Clear();
    pointLightComponent.Clear();
    audioPlayerComponent.Clear();
}

GameObject::~GameObject() {
    // DebugLogInfo("Destructor was called on ", name, " (", this, ").");    
};

// TransformComponent* GameObject::LuaGetTransform() {
    // return transformComponent.GetPtr(); // TODO WHY WOULD I WRITE THAT WHAT IS WRONG WITH ME LUA SHOULD UNDER NO CIRCUMSTANCES HAVE RAW PTRS
// }

GameObject::GameObject(const GameobjectCreateParams& params, std::array<void*, ComponentRegistry::N_COMPONENT_TYPES> components):
    // a way to make this less verbose and more type safe would be nice
    transformComponent((TransformComponent*)components[ComponentRegistry::TransformComponentBitIndex]),
    renderComponent((GraphicsEngine::RenderComponent*)components[ComponentRegistry::RenderComponentBitIndex] ? (GraphicsEngine::RenderComponent*)components[ComponentRegistry::RenderComponentBitIndex] : (GraphicsEngine::RenderComponentNoFO*)components[ComponentRegistry::RenderComponentNoFOBitIndex]),  
    rigidbodyComponent((RigidbodyComponent*)components[ComponentRegistry::RigidbodyComponentBitIndex]),
    colliderComponent((SpatialAccelerationStructure::ColliderComponent*)components[ComponentRegistry::ColliderComponentBitIndex]),
    pointLightComponent((PointLightComponent*)components[ComponentRegistry::PointlightComponentBitIndex]),
    audioPlayerComponent((AudioPlayerComponent*)components[ComponentRegistry::AudioPlayerComponentBitIndex])
{
    
    assert(transformComponent); // if you want to make transform component optional, ur gonna have to mess with the postfix/prefix operators of the iterator (but lets be real, we always gonna have a transform component)
    transformComponent->Init();
    if (renderComponent) {
        assert(Mesh::Get(params.meshId)); // verify that we were given a valid meshId
        renderComponent->Init(params.meshId, params.materialId, params.shaderId != 0 ? params.shaderId: GraphicsEngine::Get().defaultShaderProgram->shaderProgramId);
    }
    if (colliderComponent) {
        std::shared_ptr<PhysicsMesh> physMesh;
        if (params.physMeshId == 0) {
            if (params.meshId == 0) {
                std::cout << "ERROR: When trying to create a ColliderComponent, no PhysicsMeshId was given, and no MeshId was given to produce one with.\n";
                abort();
            }
            physMesh = PhysicsMesh::New(Mesh::Get(params.meshId));
        }
        else {
            physMesh = PhysicsMesh::Get(params.physMeshId);
        }
        colliderComponent->Init(this, physMesh);
    };
    if (rigidbodyComponent) {rigidbodyComponent->Init();}
    if (pointLightComponent) {pointLightComponent->Init();}
    if (audioPlayerComponent) {audioPlayerComponent->Init(this, params.sound);}
    name = "GameObject";
};
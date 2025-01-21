#include "gameobject.hpp"
#include "component_field_initializer.hpp"

// Used to make shared_ptr for GameObject, which lacks a public constructor
template <typename... Args>
inline std::shared_ptr<GameObject> protected_make_shared(Args&&... args)
{
    struct helper : public GameObject
    {
        helper(Args&&... args)
            : GameObject(std::forward< Args >(args)...)
        {}
    };

    return std::make_shared< helper >(std::forward< Args >(args)...);
}

std::tuple<ComponentPool*, int, int> GameObject::GetNewGameobjectComponentData(const GameobjectCreateParams& params) {
    if (!COMPONENT_POOLS.contains(params.requestedComponents)) {
        COMPONENT_POOLS.emplace(params.requestedComponents, new ComponentPool(params.requestedComponents));
    }

    std::unique_ptr<ComponentPool>& pool = COMPONENT_POOLS.at(params.requestedComponents);
    auto [components, page, objectIndex] = pool->GetObject(); // TODO: components return is pointless
    return std::make_tuple(pool.get(), page, objectIndex);
}

std::shared_ptr<GameObject> GameObject::New(const GameobjectCreateParams& params) {
    
    

    // we NEVER delete pools except when program ends (before which all gameobjects are destroyed) so this is fine
    auto ptr = protected_make_shared(params);

    GAMEOBJECTS()[ptr.get()] = ptr;

    return ptr;
}

GameObject::GameObject(const GameobjectCreateParams& params):
    GameObject(GetNewGameobjectComponentData(params))
{
    int materialId = params.materialId == 0 ? GraphicsEngine::Get().defaultMaterial->id : params.materialId;
    Assert(materialId != 0);

    // TODO: can the transform comp restriction ever be lifted?
    Assert(params.requestedComponents[ComponentBitIndex::Transform]);
    if (params.requestedComponents[ComponentBitIndex::Transform]) {
        auto ptr = RawGet<TransformComponent>();
        std::construct_at(ptr);
    }
    // can't have both kinds of rendercomponent
    Assert((params.requestedComponents[ComponentBitIndex::Render] && params.requestedComponents[ComponentBitIndex::RenderNoFO]) == false);
    if (params.requestedComponents[ComponentBitIndex::Render] || params.requestedComponents[ComponentBitIndex::RenderNoFO]) {
        Assert(Mesh::Get(params.meshId)); // verify that we were given a valid meshId
        std::construct_at(RawGet<RenderComponent>(), params.meshId, materialId);
    }
    if (params.requestedComponents[ComponentBitIndex::Collider] || params.requestedComponents[ComponentBitIndex::Rigidbody]) {
        std::shared_ptr<PhysicsMesh> physMesh;

        if (params.physMesh == std::nullopt) {
            if (params.meshId == 0) {
                DebugLogError("When trying to create a ColliderComponent, no PhysicsMesh was given, and no MeshId was given to produce one with.");
                abort();
            }
            // DebugLogInfo("Creating physmesh from ", params.meshId);
            physMesh = PhysicsMesh::New(Mesh::Get(params.meshId));
        }
        else {
            physMesh = params.physMesh.value();
            
        }

        Assert(physMesh != nullptr);

        if (params.requestedComponents[ComponentBitIndex::Collider]) {
            std::construct_at(RawGet<ColliderComponent>(), this, physMesh);
        }
        if (params.requestedComponents[ComponentBitIndex::Rigidbody]) {
            std::construct_at(RawGet<RigidbodyComponent>(), physMesh);
        }
    };
    
    if (params.requestedComponents[ComponentBitIndex::Pointlight]) {
        std::construct_at(RawGet<PointLightComponent>());
    }
    
    if (params.requestedComponents[ComponentBitIndex::AudioPlayer]) {
        std::construct_at(RawGet<AudioPlayerComponent>(), params.sound);
    }

    if (params.requestedComponents[ComponentBitIndex::Animation]) {
        Assert(params.requestedComponents[ComponentBitIndex::Render]); // TODO: RenderNoFO???? How????
        std::construct_at(RawGet<AnimationComponent>(), RawGet<RenderComponent>());
    }
    if (params.requestedComponents[ComponentBitIndex::Spotlight]) {
        std::construct_at(RawGet<SpotLightComponent>());
    }

    //for (auto& initializer : params.componentFieldInitializers) {
        //initializer->Apply();
    //}
}

GameObject::GameObject(std::tuple<ComponentPool*, int, int> data) : 
    name("GameObject"),
    pool(std::get<0>(data)),
    objectIndex(std::get<2>(data)),
    page(std::get<1>(data))
{
    Assert(pool != nullptr);
    Assert(pool->pages.size() > page);

}

GameObject::~GameObject() {
    if (MaybeRawGet<TransformComponent>()) {
        std::destroy_at(RawGet<TransformComponent>());
    }
    if (MaybeRawGet<RenderComponent>()) {
        std::destroy_at(RawGet<RenderComponent>());
    }
    if (MaybeRawGet<RigidbodyComponent>()) {
        std::destroy_at(RawGet<RigidbodyComponent>());
    }
    if (MaybeRawGet<ColliderComponent>()) {
        std::destroy_at(RawGet<ColliderComponent>());
    }
    if (MaybeRawGet<AnimationComponent>()) {
        std::destroy_at(RawGet<AnimationComponent>());
    }
    if (MaybeRawGet<PointLightComponent>()) {
        std::destroy_at(RawGet<PointLightComponent>());
    }
    if (MaybeRawGet<SpotLightComponent>()) {
        std::destroy_at(RawGet<SpotLightComponent>());
    }
    if (MaybeRawGet<AudioPlayerComponent>()) {
        std::destroy_at(RawGet<AudioPlayerComponent>());
    }

    pool->ReturnObject(page, objectIndex);
}

void GameObject::Destroy() {
    // TODO
    Assert(GAMEOBJECTS().count(this));
    GAMEOBJECTS().erase(this);
}

std::unordered_map<GameObject*, std::shared_ptr<GameObject>>& GameObject::GAMEOBJECTS() {
    static std::unordered_map<GameObject*, std::shared_ptr<GameObject>> map;
    return map;
}
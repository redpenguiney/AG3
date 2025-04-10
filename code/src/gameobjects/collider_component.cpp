#include "physics/spatial_acceleration_structure.hpp"
#include "gameobjects/collider_component.hpp"
#include "gameobjects/gameobject.hpp"

#include "physics/gjk.hpp"

ColliderComponent::ColliderComponent(GameObject* gameobj, std::shared_ptr<PhysicsMesh>& physMesh):
    gameobject(gameobj),
    physicsMesh(physMesh)
{
    Assert(gameobject);

    aabbType = AABBBoundingCube;
    node = nullptr;
    
    elasticity = 1.0f;
    friction = 0.2f;
    density = 1.0f;
    SpatialAccelerationStructure::Get().AddCollider(this, *gameobject->RawGet<TransformComponent>());

}

//ColliderComponent::ColliderComponent() {
//    // no point in initializing but makes msvc shut up
//    density = 0;
//    elasticity = 0;
//    friction = 0;
//    gameobject = nullptr;
//    physicsMesh = nullptr;
//    node = nullptr;
//}

ColliderComponent::~ColliderComponent() {
    RemoveFromSas(); // TODO: confusion: even without this line Query() doesn't pick up these components???
}

std::shared_ptr<GameObject> ColliderComponent::GetGameObject() {
    //Assert(GameObject::GAMEOBJECTS().contains(gameobject));
    
    return gameobject->shared_from_this();
}

// bool ColliderComponent::IsCollidingWith(const ColliderComponent& other) const {
//     TODO
// }

// TODO: collider AABBs should be augmented to contain their motion over the next time increment.
    // If we ever use a second SAS for accelerating visibility queries too, then don't do it for that
void ColliderComponent::RecalculateAABB(const TransformComponent& colliderTransform) {
    // std::cout << "Reacalculating AABB of " << this << "\n";
    if (aabbType == AABBBoundingCube) {
        glm::dvec3 min = {-std::sqrt(0.75), -std::sqrt(0.75), -std::sqrt(0.75)};
        glm::dvec3 max = {std::sqrt(0.75), std::sqrt(0.75), std::sqrt(0.75)};
        
        // TODO: maybe fat factor should be added instead of multiplied?
        min *= AABB_FAT_FACTOR;
        min *= glm::compMax(colliderTransform.Scale());
        max *= AABB_FAT_FACTOR;
        max *= glm::compMax(colliderTransform.Scale());

        min += colliderTransform.Position();
        max += colliderTransform.Position();
        aabb = AABB(min, max);
    }
    else {
        DebugLogError("PROBLEM");
        abort();
    }
}

const AABB& ColliderComponent::GetAABB() {
    return aabb;
}

CollisionLayer ColliderComponent::GetCollisionLayer()
{
    return layer;
}

void ColliderComponent::SetCollisionLayer(CollisionLayer newLayer) {
    Assert(newLayer < MAX_COLLISION_LAYERS);
    auto old = layer;
    layer = newLayer;
    SpatialAccelerationStructure::Get().UpdateColliderLayer(*this, old);
}

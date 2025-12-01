#include "physics/spatial_acceleration_structure.hpp"
#include "gameobjects/collider_component.hpp"
#include "gameobjects/gameobject.hpp"

#include "physics/gjk.hpp"

ColliderComponent::ColliderComponent(GameObject* gameobj, std::shared_ptr<PhysicsMesh>& physMesh):
    gameobject(gameobj),
    physicsMesh(physMesh),
    aabbType(BoundingBox)
{
    Assert(gameobject);

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
        glm::dvec3 min(-std::sqrt(0.75)), max(std::sqrt(0.75));
        
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

        glm::vec3 min(-0.5f, -0.5f, -0.5f);
        for (float x : { -0.5f, 0.5f }) {
            for (float y : { -0.5f, 0.5f }) {
                for (float z : { -0.5f, 0.5f }) {
                    glm::vec3 transformed = colliderTransform.Rotation() * glm::vec3(x, y, z);
                    min.x = std::min(min.x, transformed.x);
                    min.y = std::min(min.y, transformed.y);
                    min.z = std::min(min.z, transformed.z);
                }
            }
        }

        min *= colliderTransform.Scale();
        glm::vec3 max = -min;

        min *= AABB_FAT_FACTOR;
        max *= AABB_FAT_FACTOR;
        min += colliderTransform.Position();
        max += colliderTransform.Position();
        aabb = AABB(min, max);

        //DebugPlacePointOnPosition(min, {1, 0, 0, 1});
        //DebugPlacePointOnPosition(max, {1, 0, 1, 1});
    }
}

AABB ColliderComponent::GetTightfittingAABB(const TransformComponent& colliderTransform) {
    if (aabbType == BoundingBox) {
        return aabb;
    }
    else {
        glm::vec3 min(-0.5);

        min *= colliderTransform.Scale();
        min = colliderTransform.Rotation() * min;
        glm::vec3 max = -min;

        min *= AABB_FAT_FACTOR;
        max *= AABB_FAT_FACTOR;
        min += colliderTransform.Position();
        max += colliderTransform.Position();

        return AABB(min, max);
    }
}

const AABB& ColliderComponent::GetAABB() {
    return aabb;
}

bool ColliderComponent::IsCollidingWith(ColliderComponent& other) {
    RecalculateAABB(*gameobject->RawGet<TransformComponent>());
    other.RecalculateAABB(*other.gameobject->RawGet<TransformComponent>());

    if (aabb.TestIntersection(other.aabb)) {
        return IsColliding(*gameobject->RawGet<TransformComponent>(), *this, *other.gameobject->RawGet<TransformComponent>(), other).has_value();
    }
    return false;
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

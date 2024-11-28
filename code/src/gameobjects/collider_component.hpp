#pragma once
#include "physics/aabb.hpp"
#include "physics/spatial_acceleration_structure.hpp"
#include <bitset>

class PhysicsMesh;
class GameObject;

// Represents what kind of AABB to generate for a collider.
// BoundingCube AABB will create a cube-shaped AABB that will contain the object no matter how it is rotated, which is fast to make/update but for a long skinny object will create a lot of false collisions for the expensive narrow phase to deal with.
// BoundingBox AABB will tightly fit the object at all times regardless of rotation, which will be a bit slower but for a long skinny object might save performance.
enum BroadPhaseAABBType {
    AABBBoundingCube = 0,
    //BoundingBox = TODO
};

class ColliderComponent: public BaseComponent {
    public:
    BroadPhaseAABBType aabbType;


    // how bouncy something is, should probably be between 0 and 1 but knock urself out
    float elasticity; 

    // TODO: currently completely unrelated to mass, just used to compute resting contact forces/linear slop
    float density; 

    // duh
    float friction;

    // Called when collider is gotten from pool
    //void Init(GameObject* gameobject, std::shared_ptr<PhysicsMesh>& physMesh);

    ColliderComponent(const ColliderComponent&) = delete;
    ColliderComponent(GameObject* gameobject, std::shared_ptr<PhysicsMesh>& physMesh);
    ~ColliderComponent();

    // Called before component is returned from pool
    //void Destroy();

    // Removes the collider from the spatial acceleration structure, meaning it will no longer do collisions.
    void RemoveFromSas();

    // recalculate AABB of collider component from its transform
    void RecalculateAABB(const TransformComponent& colliderTransform);

    //TransformComponent* transform;

    // returns gameobject this collider belongs to; TODO does this really need to be shared_ptr
    std::shared_ptr<GameObject>& GetGameObject();

    // pointer to accurate collider for object
    const std::shared_ptr<PhysicsMesh> physicsMesh;

    const AABB& GetAABB();

    // Returns true if the object is colliding with the given other object.
    bool IsCollidingWith(const ColliderComponent& other) const;

    // Returns all colliders the collider is currently intersecting.
    std::vector<ColliderComponent*> GetColliding() const;

    // pointer to gameobject using this collier. No, no way around this, we have to be able to get gameobjects from a collider stored in the SAS.
    // The lifetime of this pointer is as long as the lifetime of the component. Use GetGameObject() if you want to make sure the object doesn't get deleted while you're using it.
    GameObject* const gameobject;

    CollisionLayer GetCollisionLayer();
    void SetCollisionLayer(CollisionLayer newLayer);

    private:

    CollisionLayer layer = 0;

    AABB aabb;

    // pointer to node the component is stored in
    SpatialAccelerationStructure::SasNode* node;

    
    
    // private constructor to enforce usage of object pool
    //friend class ComponentPool<ColliderComponent>;
    friend class SpatialAccelerationStructure;
    

};
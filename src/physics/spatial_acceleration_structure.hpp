#pragma once
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <vector>
#include <array>
#include "../../external_headers/GLM/ext.hpp"
#include "../gameobjects/transform_component.hpp"
#include "physics_mesh.hpp"

class GameObject;

// For the broadphase of collisions, we use AABBs since it is easier to determine when they are colliding
struct AABB {
    glm::dvec3 min;
    glm::dvec3 max;

    AABB(glm::dvec3 minPoint = {}, glm::dvec3 maxPoint = {}) {
        min = minPoint;
        max = maxPoint;
    }

    // makes this aabb expand to contain other
    void Grow(const AABB& other) {
        min.x = std::min(min.x, other.min.x);
        min.y = std::min(min.y, other.min.y);
        min.z = std::min(min.z, other.min.z);
        max.x = std::max(max.x, other.max.x);
        max.y = std::max(max.y, other.max.y);
        max.z = std::max(max.z, other.max.z);
    }

    // Checks if this AABB fully envelopes the other AABB 
    bool TestEnvelopes(const AABB& other) {
        return (min.x <= other.min.x && min.y <= other.min.y && min.z <= other.min.z && max.x >= other.max.x && max.y >= other.max.y && max.z >= other.max.z);
    }

    // returns true if they touching
    bool TestIntersection(const AABB& other) {
        return (min.x <= other.max.x && max.x >= other.min.x) && (min.y <= other.max.y && max.y >= other.min.y) && (min.z <= other.max.z && max.z >= other.min.z);
    }

    // returns true if they touching
    // uses https://tavianator.com/2011/ray_box.html 
    bool TestIntersection(const glm::dvec3& origin, const glm::dvec3& direction_inverse) {
        //std::printf("Testing AABB going from %f %f %f to %f %f %f\n.", min.x, min.y, min.z, max.x, max.y, max.z);

        double t1 = (min[0] - origin[0]) * direction_inverse[0];
        double t2 = (max[0] - origin[0]) * direction_inverse[0];

        double tmin = std::min(t1, t2);
        double tmax = std::max(t1, t2);

        for (int i = 1; i < 3; ++i) {
            t1 = (min[i] - origin[i]) * direction_inverse[i];
            t2 = (max[i] - origin[i]) * direction_inverse[i];

            tmin = std::min(std::max(t1, tmin), std::max(t2, tmin));
            tmax = std::max(std::min(t1, tmax), std::min(t2, tmax));
        }

        return tmax > std::max(tmin, 0.0);
    }

    // returns average of min and max
    glm::dvec3 Center() const {
        return (min + max) * 0.5;
    }

    // returns AABB's volume
    double Volume() const {
        auto m = max - min;
        return m.x * m.y * m.z;
    }   
};


// A dynamic AABB tree structure that allows for fast queries of objects by position, which is needed for collision detection that isn't O(n^2).
// Specifically used for broad phase collision detection.
// based on https://www.cs.nmsu.edu/~joshagam/Solace/papers/master-writeup-print.pdf
class SpatialAccelerationStructure { // (SAS)
    public:
    SpatialAccelerationStructure(SpatialAccelerationStructure const&) = delete; // no copying
    SpatialAccelerationStructure& operator=(SpatialAccelerationStructure const&) = delete; // no assigning

    static SpatialAccelerationStructure& Get()
    {
        static SpatialAccelerationStructure sas; // yeah apparently you can have local static variables
        return sas;
    }

    // Represents what kind of AABB to generate for a collider.
    // BoundingCube AABB will create a cube-shaped AABB that will contain the object no matter how it is rotated, which is fast to make/update but for a long skinny object will create a lot of false collisions for the expensive narrow phase to deal with.
    // BoundingBox AABB will tightly fit the object at all times regardless of rotation, which will be a bit slower but for a long skinny object will save performance.
    enum BroadPhaseAABBType {
        AABBBoundingCube = 0,
        //BoundingBox = TODO
    };

    private:
    struct SasNode;
    public:

    class ColliderComponent: public BaseComponent<ColliderComponent> {
        public:
        BroadPhaseAABBType aabbType;

        // how bouncy something is, should probably be between 0 and 1 but knock urself out
        float elasticity; 

        // TODO: currently completely unrelated to mass, just used to compute resting contact forces/linear slop
        float density; 

        // Called when collider is gotten from pool
        void Init(GameObject* gameobject, std::shared_ptr<PhysicsMesh>& physMesh);

        // Called before component is returned from pool
        void Destroy();

        // Removes the collider from the spatial acceleration structure, meaning it will no longer do collisions.
        void RemoveFromSas();

        // recalculate AABB of collider component from its transform
        void RecalculateAABB(const TransformComponent& colliderTransform);

        //TransformComponent* transform;

        // returns gameobject this collider belongs to; TODO does this really need to be shared_ptr
        std::shared_ptr<GameObject>& GetGameObject();

        // pointer to accurate collider for object
        std::shared_ptr<PhysicsMesh> physicsMesh;

        const AABB& GetAABB();

        private:

        AABB aabb;

        // pointer to node the component is stored in
        SasNode* node;

        // pointer to gameobject using this collier. No, no way around this, we have to be able to get gameobjects from a collider stored in the SAS.
        GameObject* gameobject;
        friend class GameObject; // gameobject has to set the gameobject ptr after creating the collider for annoying reasons
        
        // private constructor to enforce usage of object pool
        friend class ComponentPool<ColliderComponent>;
        friend class SpatialAccelerationStructure;
        ColliderComponent() {}
        ~ColliderComponent() {}
    
        };

    // Call every frame. Updates the SAS to use the most up-to-date object transforms.
    void Update();

    // Returns the set of colliders whose AABBs intersect the given AABB (assuming the colliders are in the SAS, which they should be).
    std::vector<ColliderComponent*> Query(const AABB& collider);

    // Returns the set of colliders whose AABBs intersect the given ray (assuming the colliders are in the SAS, which they should be).
    std::vector<ColliderComponent*> Query(const glm::dvec3& origin, const glm::dvec3& direction);

    // Adds a collider to the SAS.
    void AddCollider(ColliderComponent* collider, const TransformComponent& transform);

    // some lame unabstracted opengl code that visualizes the SAS, for the purposes of debugging the SAS.
    void DebugVisualize();

    private:

    // Helper function for DebugVisualize(), disregard.
    void DebugVisualizeAddVertexAttributes(SasNode const& node, std::vector<float>& instancedVertexAttributes, unsigned int& numInstances, unsigned int depth=0);

    // recursive helper functions for Query(), ignore (member func because SasNode is private)
    void AddIntersectingLeafNodes(SasNode* node, std::vector<SasNode*>& collidingNodes, const AABB& collider);
    void AddIntersectingLeafNodes(SasNode* node, std::vector<SasNode*>& collidingNodes, const glm::dvec3& origin, const glm::dvec3& inverse_direction);

    // call whenever collider moves or changes size
    void UpdateCollider(ColliderComponent& collider, const TransformComponent& transform);

    // AABBs inserted into the SAS will be scaled by this much so that if the object moves a little bit we don't need to update its position in the SAS.
    static const inline double AABB_FAT_FACTOR = 1;
    
    // Once there are more objects in a node than this threshold, the node splits
    static const inline unsigned int NODE_SPLIT_THRESHOLD = 50;
    
    SpatialAccelerationStructure();
    ~SpatialAccelerationStructure();

    // Node in the SAS's dynamic AABB tree
    struct SasNode {
        glm::dvec3 splitPoint; // point that splits the aabb to determine aabbs of children, initialized to (nan, nan, nan) 

        bool split; // when this node is queried and it has more objects than NODE_SPLIT_THRESHOLD and this bool is false, the node will be split.
            // however, if the node can't be split (because all objects are in same position or something) then we'll just set this bool to true and then set it to false when a new collider is added that might make the node splittable
        std::array<SasNode*, 27>* children;
        // children are stored in an icoseptree. index with [x * 9 + y * 3 + z], assuming 0 = lower coord 1 = middle 2 = higher coord

        std::vector<ColliderComponent*> objects;
        SasNode* parent;
        AABB aabb; // aabb that contains all objects/children of the node

        SasNode();

        // recalculate AABB of node from its contents
        void RecalculateAABB();

        // sets splitPoint, calculated by mean position of objects inside, creates children nodes, and moves objects into children nodes
        void Split();

    };

    // Returns best child node to insert object into, given the parent node and object's AABB.
    // Returns nullptr if no child node.    
    static SasNode* SasInsertHeuristic(SasNode& node, const AABB& aabb);

    SasNode root;
};
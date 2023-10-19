#pragma once
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <vector>
#include <array>
#include "../../external_headers/GLM/ext.hpp"
#include "../gameobjects/transform_component.cpp"

class GameObject;

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
        double t1 = (min[0] - origin[0])*direction_inverse[0];
        double t2 = (max[0] - origin[0])*direction_inverse[0];

        double tmin = std::min(t1, t2);
        double tmax = std::max(t1, t2);

        for (int i = 1; i < 3; ++i) {
            t1 = (min[i] - origin[i])*direction_inverse[i];
            t2 = (max[i] - origin[i])*direction_inverse[i];

            tmin = std::max(tmin, std::min(t1, t2));
            tmax = std::min(tmax, std::max(t1, t2));
        }

        std::printf("min %f max %f \n", tmin, tmax);
        std::printf("dirinv %f %f %f\n", direction_inverse.x, direction_inverse.y, direction_inverse.z);
        return tmax > std::max(tmin, 0.0);
    }

    // returns average of min an max
    glm::dvec3 Center() {
        return (min + max) * 0.5;
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

    class ColliderComponent: public BaseComponent {
        public:
        BroadPhaseAABBType aabbType;

        // DO NOT delete this pointer.
        // The gameobject argument kinda has to be a nullptr and then you manually set the field to the shared_ptr, so this is kinda dumb. TODO
        static ColliderComponent* New(std::shared_ptr<GameObject> gameobject);

        // call instead of deleting the pointer.
        // obviously don't touch component after this.
        void Destroy();

        // recalculate AABB of collider component from its transform
        void RecalculateAABB(const TransformComponent& colliderTransform);

        //TransformComponent* transform;

        // returns gameobject this collider belongs to
        std::shared_ptr<GameObject>& GetGameObject();

        private:

        AABB aabb;

        // pointer to node the component is stored in
        SasNode* node;

        // pointer to gameobject using this collier. No, no way around this, we have to be able to get gameobjects from a collider stored in the SAS.
        std::shared_ptr<GameObject> gameobject;
        friend class GameObject; // gameobject has to set the gameobject ptr after creating the collider for annoying reasons
        
        // private constructor to enforce usage of object pool
        friend class ComponentPool<ColliderComponent>;
        friend class SpatialAccelerationStructure;
        ColliderComponent() {}
        ~ColliderComponent() {}
        
        // object pool
        static inline ComponentPool<ColliderComponent> COLLIDER_COMPONENTS;
    };

    // Call every frame. Updates the SAS to use the most up-to-date object transforms.
    void Update();

    // Returns a vector of colliders in the SAS that might intersect the given AABB.
    std::vector<ColliderComponent*> Query(const AABB& collider);

    // Returns a vector of colliders in the SAS that might intersect the given ray.
    std::vector<ColliderComponent*> Query(const glm::dvec3& origin, const glm::dvec3& direction);

    // Adds a collider to the SAS.
    void AddCollider(ColliderComponent* collider, const TransformComponent& transform);

    // Duh.
    void RemoveCollider();

    private:

    // recursive helper functions for Query(), ignore (member func because SasNode is private)
    void AddIntersectingLeafNodes(SasNode* node, std::vector<SasNode*>& collidingNodes, const AABB& collider);
    void AddIntersectingLeafNodes(SasNode* node, std::vector<SasNode*>& collidingNodes, const glm::dvec3& origin, const glm::dvec3& inverse_direction);

    // call whenever collider moves or changes size
    void UpdateCollider(ColliderComponent& collider, const TransformComponent& transform);

    // AABBs inserted into the SAS will be scaled by this much so that if the object moves a little bit we don't need to update its position in the SAS.
    static const inline double AABB_FAT_FACTOR = 1;
    
    // Once there are more objects in a node than this threshold, the node splits
    static const inline unsigned int NODE_SPLIT_THRESHOLD = 60;
    
    SpatialAccelerationStructure();
    ~SpatialAccelerationStructure();

    // Node in the SAS's dynamic AABB tree
    struct SasNode {
        glm::dvec3 splitPoint; // point that splits the aabb to determine aabbs of children

        bool split; // when this node is queried and it has more objects than NODE_SPLIT_THRESHOLD and this bool is false, the node will be split.
            // however, if the node can't be split (because all objects are in same position or something) then we'll just set this bool to true and then set it to false when a new collider is added that might make the node splittable
        std::array<SasNode*, 27>* children;
        // children are stored in an icoseptree. index with [x * 9 + y * 3 + z], assuming 0 = lower coord 1 = middle 2 = higher coord

        std::vector<ColliderComponent*> objects;
        SasNode* parent;
        AABB aabb; // aabb that contains all objects/children of the node

        SasNode();

        void CalculateAABB();

        // sets splitPoint, calculated by mean position of objects inside, creates children nodes, and moves objects into children nodes
        void UpdateSplitPoint();

    };

    // Returns index of best child node to insert object into, given the parent node and object's AABB
    static int SasInsertHeuristic(const SasNode& node, const AABB& aabb);

    SasNode root;
};
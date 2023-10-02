#pragma once
#include <cassert>
#include <vector>
#include <array>
#include "../../external_headers/GLM/ext.hpp"
#include "../gameobjects/transform_component.cpp"

struct AABB {
    glm::dvec3 min;
    glm::dvec3 max;
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

    class ColliderComponent: public BaseComponent {
        public:
        const BroadPhaseAABBType aabbType;

        // DO NOT delete this pointer.
        static ColliderComponent* New(TransformComponent* transformComponent) {
            
            auto ptr = COLLIDER_COMPONENTS.GetNew();
            ptr->transform = transformComponent;
            SpatialAccelerationStructure::Get().AddCollider(*ptr);
            return ptr;
        }

        // call instead of deleting the pointer.
        // obviously don't touch component after this.
        void Destroy() {
            COLLIDER_COMPONENTS.ReturnObject(this);
        }

        // Return AABB of collider component
        AABB GetAABB() {
            if (aabbType == AABBBoundingCube) {
                glm::dvec3 min = {-std::sqrt(0.75), -std::sqrt(0.75), -std::sqrt(0.75)};
                glm::dvec3 max = {std::sqrt(0.75), std::sqrt(0.75), std::sqrt(0.75)};
                min *= transform->scale() * AABB_FAT_FACTOR;
                max *= transform->scale() * AABB_FAT_FACTOR;
                min += transform->position;
                max += transform->position;
            }
            else {
                assert(false);
            }
        }

        private:
        // AABBs inserted into the SAS will be scaled by this much so that if the object moves a little bit we don't need to update its position in the SAS.
        static const inline double AABB_FAT_FACTOR = 1.2;
        
        // Once there are more objects in a node than this threshold, the node splits
        static const inline unsigned int NODE_SPLIT_THRESHOLD = 60;

        TransformComponent* transform;

        // private constructor to enforce usage of object pool
        friend class ComponentPool<ColliderComponent, 65536>;
        ColliderComponent(): aabbType(AABBBoundingCube) {}
        ~ColliderComponent() {}
        
        // object pool
        static inline ComponentPool<ColliderComponent, 65536> COLLIDER_COMPONENTS;
    };

    // void Query() {

    // }

    // Adds a collider to the SAS
    void AddCollider(ColliderComponent& collider) {
        // 
    }

    // Duh.
    void RemoveCollider() {

    }

    // call whenever collider moves or changes size
    void UpdateCollider() {

    }

    private:
    
    SpatialAccelerationStructure() {
        root = SasNode();
    }

    ~SpatialAccelerationStructure() {

    }

    // Node in the SAS's dynamic AABB tree
    struct SasNode {
        bool split; // when this node is queried and it has more objects than NODE_SPLIT_THRESHOLD and this bool is false, the node will be split.
            // however, if the node can't be split (because all objects are in same position or something) then we'll just set this bool to true and then set it to false when a new collider is added that might make the node splittable
        std::array<SasNode*, 27>* children;
        // children are stored in an icoseptree. index with [x * 9 + y * 3 + z], assuming 0 = left 1 = middle 2 = right

        std::vector<ColliderComponent*> objects;
        SasNode* parent;
        AABB aabb; // aabb that contains all objects/children of the node

        SasNode() {
            split = false;
            parent = nullptr;
            children = nullptr;
        }
    };

    SasNode root;
};
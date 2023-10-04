#pragma once
#include <cassert>
#include <cstddef>
#include <vector>
#include <array>
#include "../../external_headers/GLM/ext.hpp"
#include "../gameobjects/transform_component.cpp"

struct AABB {
    glm::dvec3 min;
    glm::dvec3 max;

    AABB(glm::dvec3 minPoint = {}, glm::dvec3 maxPoint = {}) {
        min = minPoint;
        max = maxPoint;
    }

    // makes this aabb expand to contain other
    void Grow(AABB& other) {
        min.x = std::min(min.x, other.min.x);
        min.y = std::min(min.y, other.min.y);
        min.z = std::min(min.z, other.min.z);
        max.x = std::max(max.x, other.max.x);
        max.y = std::max(max.y, other.max.y);
        max.z = std::max(max.z, other.max.z);
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

    class ColliderComponent: public BaseComponent {
        public:
        BroadPhaseAABBType aabbType;

        // DO NOT delete this pointer.
        static ColliderComponent* New(TransformComponent* transformComponent) {
            
            auto ptr = COLLIDER_COMPONENTS.GetNew();
            ptr->transform = transformComponent;
            ptr->aabbType = AABBBoundingCube;
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
                return AABB(min, max);
            }
            else {
                assert(false);
            }
        }

        AABB aabb;
        TransformComponent* transform;

        private:
        
        // private constructor to enforce usage of object pool
        friend class ComponentPool<ColliderComponent, 65536>;
        ColliderComponent() {}
        ~ColliderComponent() {}
        
        // object pool
        static inline ComponentPool<ColliderComponent, 65536> COLLIDER_COMPONENTS;
    };

    // Returns a vector of AABBs in the SAS that intersect the given AABB
    std::vector<AABB> Query(AABB collider) {

    }

    // Adds a collider to the SAS.
    void AddCollider(ColliderComponent& collider) {
        

    }

    // Duh.
    void RemoveCollider() {

    }

    // call whenever collider moves or changes size
    void UpdateCollider() {

    }

    private:

    // AABBs inserted into the SAS will be scaled by this much so that if the object moves a little bit we don't need to update its position in the SAS.
    static const inline double AABB_FAT_FACTOR = 1.2;
    
    // Once there are more objects in a node than this threshold, the node splits
    static const inline unsigned int NODE_SPLIT_THRESHOLD = 60;
    
    SpatialAccelerationStructure() {
        root = SasNode();
    }

    ~SpatialAccelerationStructure() {

    }

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

        SasNode() {
            
            split = false;
            parent = nullptr;
            children = nullptr;
        }

        void calculateAABB() {
            // first find a single object or child node that has an aabb, and use that as a starting point
            if (objects.size() > 0) {
                aabb = objects[0]->aabb;
            }
            else if (children != nullptr) {
                bool found = false;
                for (auto & child: *children) {
                    if (child != nullptr) {
                        aabb = child->aabb;
                        found = true;
                        break;
                    }
                }
                assert(found);
            }

            // now aabb is within the real aabb, we get to real aabb by growing aabb to contain every child node and object
            for (auto & obj: objects) {
                aabb.Grow(obj->aabb);
            }
            if (children != nullptr) {
                for (auto & child: *children) {
                    if (child != nullptr) {
                        aabb.Grow(child->aabb);
                    }
                }
            }

        }

        // sets splitPoint, calculated by mean position of objects inside, creates children nodes, and moves objects into children nodes
        void UpdateSplitPoint() {
            assert(objects.size() >= NODE_SPLIT_THRESHOLD);
            assert(!split);
            glm::dvec3 meanPosition = {0, 0, 0};
            for (auto & obj : objects) {
                meanPosition += obj->transform->position/(double)objects.size();
            }
            splitPoint = meanPosition;

            children = new std::array<SasNode*, 27> {nullptr};

        }
    };

    SasNode root;
};
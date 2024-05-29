#pragma once
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <vector>
#include <array>
#include "GLM/vec3.hpp"
#include "gameobjects/transform_component.hpp"

#include "physics_mesh.hpp"
#include "physics/aabb.hpp"

class ColliderComponent;

// Once there are more objects in a node than this threshold, the node splits
static const inline unsigned int NODE_SPLIT_THRESHOLD = 50;

// A dynamic AABB tree structure that allows for fast queries of objects by position, which is needed for collision detection that isn't O(n^2).
// Specifically used for broad phase collision detection.
// based on https://www.cs.nmsu.edu/~joshagam/Solace/papers/master-writeup-print.pdf
class SpatialAccelerationStructure { // (SAS)
    public:
    SpatialAccelerationStructure(SpatialAccelerationStructure const&) = delete; // no copying
    SpatialAccelerationStructure& operator=(SpatialAccelerationStructure const&) = delete; // no assigning

    static SpatialAccelerationStructure& Get();
    
    // When modules (shared libraries) get their copy of this code, they need to use a special version of SpatialAccelerationStructure::Get().
    // This is so that both the module and the main executable have access to the same singleton. 
    // The executable will provide each shared_library with a pointer to the spatial acceleration structure.
    #ifdef IS_MODULE
    static void SetModuleSpatialAccelerationStructure(SpatialAccelerationStructure* structure);
    #endif


    
    private:
    friend class ColliderComponent;
    struct SasNode;
    public:

    

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
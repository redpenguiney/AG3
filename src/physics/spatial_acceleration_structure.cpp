#pragma once
#include "spatial_acceleration_structure.hpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <vector>

void SpatialAccelerationStructure::Update() {
    for (unsigned int i = 0; i < ColliderComponent::COLLIDER_COMPONENTS.pools.size(); i++) {
        for (unsigned int j = 0; j < ComponentPool<ColliderComponent>::COMPONENTS_PER_POOL; j++) {
            ColliderComponent& collider = ColliderComponent::COLLIDER_COMPONENTS.pools[i][j];
            TransformComponent& transform = TransformComponent::TRANSFORM_COMPONENTS.pools[i][j];
            if (collider.live) {
                if (transform.moved) {
                    transform.moved = false;
                    UpdateCollider(collider, transform);
                    
                }                
            }
        }
    }

    for (auto & pool: ColliderComponent::COLLIDER_COMPONENTS.pools) {
        for (unsigned int i = 0; i < ComponentPool<ColliderComponent>::COMPONENTS_PER_POOL; i++) {
            ColliderComponent& collider = pool[i];
            if (collider.live) {
                auto queryResult = Query(collider.aabb); // remember query result will include itself
                // glm::vec3 color = (queryResult.size() > 1) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
                //std::printf("Object is touching %u\n", queryResult.size());
            }
        }
    }
}

void SpatialAccelerationStructure::AddIntersectingLeafNodes(SpatialAccelerationStructure::SasNode* node, std::vector<SpatialAccelerationStructure::SasNode*>& collidingNodes, const AABB& collider) {
    if (node->aabb.TestIntersection(collider)) { // if this node touched the given collider, then its children may as well.
        if (node->children != nullptr) {
            for (auto& child : *node->children) {
                AddIntersectingLeafNodes(child, collidingNodes, collider);
            } 
        }
        
        if (!node->objects.empty()) {
            collidingNodes.push_back(node);
        }
    }
}

void SpatialAccelerationStructure::AddIntersectingLeafNodes(SpatialAccelerationStructure::SasNode* node, std::vector<SpatialAccelerationStructure::SasNode*>& collidingNodes, const glm::dvec3& origin, const glm::dvec3& inverse_direction) {
    if (node->aabb.TestIntersection(origin, inverse_direction)) { // if this node touched the given collider, then its children may as well.
        if (node->children != nullptr) {
            for (auto& child : *node->children) {
                AddIntersectingLeafNodes(child, collidingNodes, origin, inverse_direction);
            } 
        }
        
        if (!node->objects.empty()) {
            collidingNodes.push_back(node);
        }
    }
}

std::vector<SpatialAccelerationStructure::ColliderComponent*> SpatialAccelerationStructure::Query(const AABB& collider) {
    // find leaf nodes whose AABBs intersect the collider
    std::vector<SpatialAccelerationStructure::SasNode*> collidingNodes;
    AddIntersectingLeafNodes(&root, collidingNodes, collider);
    
    // test the aabbs of the objects inside each node and if so add them to the vector
    std::vector<SpatialAccelerationStructure::ColliderComponent*> collidingComponents;
    for (auto & node: collidingNodes) {
        for (auto & obj: node->objects) {
            if (obj->aabb.TestIntersection(collider)) {
                collidingComponents.push_back(obj);
            }
        }
    }

    return collidingComponents;
}

// TODO: redundant code in these two query functions, could improve
std::vector<SpatialAccelerationStructure::ColliderComponent*> SpatialAccelerationStructure::Query(const glm::dvec3& origin, const glm::dvec3& direction) {
    glm::dvec3 inverse_direction = glm::dvec3(1.0/direction.x, 1.0/direction.y, 1.0/direction.z); 

    // find leaf nodes whose AABBs intersect the ray
    std::vector<SpatialAccelerationStructure::SasNode*> collidingNodes;
    AddIntersectingLeafNodes(&root, collidingNodes, origin, inverse_direction);

    // test the aabbs of the objects inside each node and if so add them to the vector
    std::vector<SpatialAccelerationStructure::ColliderComponent*> collidingComponents;
    for (auto & node: collidingNodes) {
        for (auto & obj: node->objects) {
            if (obj->aabb.TestIntersection(origin, inverse_direction)) {
                collidingComponents.push_back(obj);
            }
        }
    }

    return collidingComponents;
}


void SpatialAccelerationStructure::SasNode::UpdateSplitPoint() {
    assert(objects.size() >= NODE_SPLIT_THRESHOLD);
    assert(!split);
    glm::dvec3 meanPosition = {0, 0, 0};
    for (auto & obj : objects) {
        meanPosition += obj->aabb.Center()/(double)objects.size();
    }
    splitPoint = meanPosition;

    children = new std::array<SasNode*, 27> {nullptr};

}

void SpatialAccelerationStructure::SasNode::CalculateAABB() {
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

SpatialAccelerationStructure::SasNode::SasNode() {
    split = false;
    parent = nullptr;
    children = nullptr;
}

// Returns index of best child node to insert object into, given the parent node and object's AABB.
// Returns -1 if no child node.
int SpatialAccelerationStructure::SasInsertHeuristic(const SpatialAccelerationStructure::SasNode& node, const AABB& aabb) {
    return -1;
}  

void SpatialAccelerationStructure::AddCollider(ColliderComponent* collider, const TransformComponent& transform) {
    collider->RecalculateAABB(transform);

    // Recursively pick best child node starting from root until we reach leaf node
    SasNode* currentNode = &root;
    while (true) {
        int childIndex = SasInsertHeuristic(*currentNode, collider->aabb);
        if (childIndex == -1) {
            break;
        }
        else {
            currentNode = currentNode->children->at(childIndex);
        }
    }

    // put collider in that leaf node
    currentNode->objects.push_back(collider);
    collider->node = currentNode;

    // Expand node and its ancestors to make sure they contain collider
    while (true) {
        currentNode->aabb.Grow(collider->aabb);
        if (currentNode->parent == nullptr) {
            return;
        }
        else {
            currentNode = currentNode->parent;
        }
    }
}

void SpatialAccelerationStructure::RemoveCollider() {

}

void SpatialAccelerationStructure::UpdateCollider(SpatialAccelerationStructure::ColliderComponent& collider, const TransformComponent& transform) {
    collider.RecalculateAABB(transform);
    const AABB& newAabb = collider.aabb;

    // Go up the tree from the collider's current node to find the first node that fully envelopes the collider.
    // (if collider's current node still envelops the collider, this will do nothing)
    
    SasNode* oldNode = collider.node;
    SasNode* smallestNodeThatEnvelopes = collider.node;
    while (true) {
        if (!smallestNodeThatEnvelopes->aabb.TestEnvelopes(newAabb)) {
            // If we reach the root and it doesn't fit, it will grow the root node's aabb to contain the collider.
            if (smallestNodeThatEnvelopes->parent == nullptr) {
                smallestNodeThatEnvelopes->aabb.Grow(newAabb);
                break; //obvi not gonna be any more parent nodes after this
            }
            else {
                smallestNodeThatEnvelopes = smallestNodeThatEnvelopes->parent;
            }
        }
        else {
            // we found a node that contains the collider
            break;
        }
    }

    // if the node still fits don't do anything
    if (oldNode == smallestNodeThatEnvelopes) {
        return;
    }

    // remove collider from old node
    for (unsigned int i = 0; i < oldNode->objects.size(); i++) { // TODO: O(n) time here could actually be an issue
        if (oldNode->objects[i] == &collider) {
            oldNode->objects.erase(oldNode->objects.begin() + i);
            break;
        }
    }

    // Then, we use the insert heuristic to go down child nodes until we find the best leaf node for the collider
    SasNode* newNodeForCollider = smallestNodeThatEnvelopes;
    while (true) {
        int childIndex = SasInsertHeuristic(*newNodeForCollider, newAabb);
        // TODO: we might need to grow this 
        if (childIndex == -1) { // then we're at a leaf node, add the collider to it
            collider.node = newNodeForCollider;
            newNodeForCollider->objects.push_back(&collider);
            return;
        }
        else {
            newNodeForCollider = newNodeForCollider->children->at(childIndex);
        }
    }
}

SpatialAccelerationStructure::SpatialAccelerationStructure() {
    root = SasNode();
}

SpatialAccelerationStructure::~SpatialAccelerationStructure() {

}

SpatialAccelerationStructure::ColliderComponent* SpatialAccelerationStructure::ColliderComponent::New(std::shared_ptr<GameObject> gameobject) {
    auto ptr = COLLIDER_COMPONENTS.GetNew();
    ptr->live = true;
    ptr->aabbType = AABBBoundingCube;
    ptr->node = nullptr;
    ptr->gameobject = gameobject;
    //SpatialAccelerationStructure::Get().AddCollider(ptr); not calling this because gameobject has to decide whether or not it actually wants collisions
    return ptr; 
}

void SpatialAccelerationStructure::ColliderComponent::Destroy() {
    COLLIDER_COMPONENTS.ReturnObject(this);
}

std::shared_ptr<GameObject>& SpatialAccelerationStructure::ColliderComponent::GetGameObject() {
    return gameobject;
}

// TODO: collider AABBs should be augmented to contain their motion over the next time increment.
    // If we ever use a second SAS for accelerating visibility queries too, then don't do it for that
void SpatialAccelerationStructure::ColliderComponent::RecalculateAABB(const TransformComponent& colliderTransform) {
    if (aabbType == AABBBoundingCube) {
        glm::dvec3 min = {-std::sqrt(0.75), -std::sqrt(0.75), -std::sqrt(0.75)};
        glm::dvec3 max = {std::sqrt(0.75), std::sqrt(0.75), std::sqrt(0.75)};
        
        min *= AABB_FAT_FACTOR;
        min *= colliderTransform.scale();
        max *= AABB_FAT_FACTOR;
        max *= colliderTransform.scale();

        min += colliderTransform.position();
        max += colliderTransform.position();
        aabb = AABB(min, max);
    }
    else {
        std::printf("PROBLEM\n");
        abort();
    }
}
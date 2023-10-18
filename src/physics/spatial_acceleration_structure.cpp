#pragma once
#include "spatial_acceleration_structure.hpp"
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

void SpatialAccelerationStructure::Update() {
    for (auto & pool: ColliderComponent::COLLIDER_COMPONENTS.pools) {
        for (int i = 0; i < ComponentPool<ColliderComponent>::COMPONENTS_PER_POOL; i++) {
            ColliderComponent& collider = pool[i];
            if (collider.live) {
                if (collider.transform->moved) {
                    collider.transform->moved = false;
                    UpdateCollider(collider);

                }
            }
        }
    }
}

void SpatialAccelerationStructure::SasNode::UpdateSplitPoint() {
    assert(objects.size() >= NODE_SPLIT_THRESHOLD);
    assert(!split);
    glm::dvec3 meanPosition = {0, 0, 0};
    for (auto & obj : objects) {
        meanPosition += obj->transform->position()/(double)objects.size();
    }
    splitPoint = meanPosition;

    children = new std::array<SasNode*, 27> {nullptr};

}

void SpatialAccelerationStructure::SasNode::CalculateAABB() {
    // first find a single object or child node that has an aabb, and use that as a starting point
    if (objects.size() > 0) {
        aabb = objects[0]->GetAABB();
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
        aabb.Grow(obj->GetAABB());
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

void SpatialAccelerationStructure::AddCollider(ColliderComponent* collider) {
    const AABB& aabb = collider->GetAABB();

    // Recursively pick best child node starting from root until we reach leaf node
    SasNode* currentNode = &root;
    while (true) {
        int childIndex = SasInsertHeuristic(*currentNode, aabb);
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
        currentNode->aabb.Grow(aabb);
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

void SpatialAccelerationStructure::UpdateCollider(SpatialAccelerationStructure::ColliderComponent& collider) {
    const AABB& newAabb = collider.GetAABB();

    // Go up the tree from the collider's current node to find the first node that fully envelopes the collider.
    // (if collider's current node still envelops the collider, this will do nothing)
    
    SasNode* smallestNodeThatEnvelopes = collider.node;
    while (true) {
        if (!smallestNodeThatEnvelopes->aabb.TestEnvelopes(newAabb)) {
            // If we reach the root and it doesn't fit, it will grow the root node's aabb to contain the collider.
            if (smallestNodeThatEnvelopes->parent != nullptr) {
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

    // Then, we use the insert heuristic to go down child nodes until we find the best leaf node for the collider
    SasNode* newNodeForCollider = smallestNodeThatEnvelopes;
    while (true) {
        int childIndex = SasInsertHeuristic(*newNodeForCollider, newAabb);
        // TODO: we might need to grow this 
        if (childIndex == -1) { // then we're at a leaf node, add the collider to it
            collider.node = newNodeForCollider;
            newNodeForCollider->objects.push_back(&collider);
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

SpatialAccelerationStructure::ColliderComponent* SpatialAccelerationStructure::ColliderComponent::New(TransformComponent* transformComponent) {
    auto ptr = COLLIDER_COMPONENTS.GetNew();
    ptr->live = true;
    ptr->transform = transformComponent;
    ptr->aabbType = AABBBoundingCube;
    //SpatialAccelerationStructure::Get().AddCollider(ptr); not calling this because gameobject has to decide whether or not it actually wants collisions
    return ptr; 
}

void SpatialAccelerationStructure::ColliderComponent::Destroy() {
    COLLIDER_COMPONENTS.ReturnObject(this);
}

const AABB& SpatialAccelerationStructure::ColliderComponent::GetAABB() {
    if (aabbType == AABBBoundingCube) {
        glm::dvec3 min = {-std::sqrt(0.75), -std::sqrt(0.75), -std::sqrt(0.75)};
        glm::dvec3 max = {std::sqrt(0.75), std::sqrt(0.75), std::sqrt(0.75)};
        min *= transform->scale() * AABB_FAT_FACTOR;
        max *= transform->scale() * AABB_FAT_FACTOR;
        min += transform->position();
        max += transform->position();
        aabb = AABB(min, max);
        return aabb;
    }
    else {
        std::printf("PROBLEM\n");
        abort();
    }
}
#include "spatial_acceleration_structure.hpp"
#include "../gameobjects/component_registry.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

void SpatialAccelerationStructure::Update() {
    // Get components of all gameobjects that have a transform and collider component
    auto pools = ComponentRegistry::GetSystemComponents({ComponentRegistry::ColliderComponentBitIndex, ComponentRegistry::TransformComponentBitIndex});

    for (auto & poolVec: pools) {
        auto transformComponents = (ComponentPool<TransformComponent>*)(poolVec[ComponentRegistry::TransformComponentBitIndex]);
        auto colliderComponents = (ComponentPool<ColliderComponent>*)(poolVec[ComponentRegistry::ColliderComponentBitIndex]);
        for (unsigned int i = 0; i < colliderComponents->pools.size(); i++) {
            auto colliderArray = colliderComponents->pools.at(i);
            auto transformArray = transformComponents->pools.at(i);
            for (unsigned int j = 0; j < colliderComponents->COMPONENTS_PER_POOL; j++) {
                auto colliderComp = colliderArray + j;
                auto transformComp = transformArray + j;
                if (colliderComp->live) {
                    if (transformComp->moved) {
                        transformComp->moved = false;
                        // std::cout << "Updating collider " << colliderComp << " at i=" << i <<", j=" << j << ".\n";
                        UpdateCollider(*colliderComp, *transformComp);
                        
                    }       
                }         
            }
        }
    }

    // dear god why were there two of these
    // for (auto & poolVec: pools) {
    //     auto transformComponents = (ComponentPool<TransformComponent>*)(poolVec[1]);
    //     auto colliderComponents = (ComponentPool<ColliderComponent>*)(poolVec[0]);
    //     for (unsigned int i = 0; i < colliderComponents->pools.size(); i++) {
    //         auto colliderArray = colliderComponents->pools.at(i);
    //         auto transformArray = transformComponents->pools.at(i);
    //         for (unsigned int j = 0; j < colliderComponents->COMPONENTS_PER_POOL; j++) {
    //             auto colliderComp = colliderArray + j;
    //             auto transformComp = transformArray + j;
    //             if (colliderComp->live) {
    //                 if (transformComp->moved) {
    //                     transformComp->moved = false;
    //                     UpdateCollider(*colliderComp, *transformComp);
                        
    //                 }       
    //             }         
    //         }
    //     }
    // }
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
        if (node->objects.size() > NODE_SPLIT_THRESHOLD) {
            node->Split();
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
        if (node->objects.size() > NODE_SPLIT_THRESHOLD) {
            node->Split();
        }
    }

    return collidingComponents;
}


void SpatialAccelerationStructure::SasNode::Split() {
    assert(objects.size() >= NODE_SPLIT_THRESHOLD);
    assert(!split);
    glm::dvec3 meanPosition = {0, 0, 0};
    for (auto & obj : objects) {
        meanPosition += obj->aabb.Center()/(double)objects.size();
    }
    splitPoint = meanPosition;

    children = new std::array<SasNode*, 27> {nullptr};
    for (int x = -1; x < 2; x++) {
        for (int y = -1; y < 2; y++) {
            for (int z = -1; z < 2; z++) {
                auto child = (*children)[(x + 1) * 9 + (y + 1) * 3 + z + 1];
            }
        }
    }

    std::vector<unsigned int> indicesToRemove;
    unsigned int i = 0;
    for (auto & obj: objects) { 
        auto index = SasInsertHeuristic(*this, obj->aabb);
        if (index != -1) {
            std::cout << "aoisjfoijesa " << children << "\n";
            (*children).at(index)->objects.push_back(obj);   
            indicesToRemove.push_back(i);
        }
        i++;
    }

    // iterate backwards through indicesToRemove to preserve index correctness
    if (indicesToRemove.size() > 0) {
        for (unsigned int i = indicesToRemove.size() - 1; i >= 0; i++) {
            indicesToRemove.erase(indicesToRemove.begin() + i);
        }
    }
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
    splitPoint = {NAN, NAN, NAN};
    parent = nullptr;
    children = nullptr;
}


int SpatialAccelerationStructure::SasInsertHeuristic(const SpatialAccelerationStructure::SasNode& node, const AABB& aabb) {
    if (node.children == nullptr) {return -1;}
    auto splitPoint = node.splitPoint;
    unsigned int x = 1, y = 1, z = 1;
    if (aabb.min.x > splitPoint.x) {
        x += 1;
    } 
    if (aabb.max.x < splitPoint.x) {
        x -= 1;
    }
    if (aabb.min.y > splitPoint.y) {
        y += 1;
    } 
    if (aabb.max.y < splitPoint.y) {
        y -= 1;
    }
    if (aabb.min.z > splitPoint.z) {
        z += 1;
    } 
    if (aabb.max.z < splitPoint.z) {
        z -= 1;
    }

    return x * 9 + y * 3 + z;
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

void SpatialAccelerationStructure::ColliderComponent::RemoveFromSas() {
    // remove object from node
    for (unsigned int i = 0; i < node->objects.size(); i++) {
        if (node->objects[i] == this) {
            node->objects.erase(node->objects.begin() + i);
            break;
        }
    }

    // TODO: we need to do something when node is emptied, probably    
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

void SpatialAccelerationStructure::ColliderComponent::Init(GameObject* gameobj, std::shared_ptr<PhysicsMesh>& physMesh) {
    aabbType = AABBBoundingCube;
    node = nullptr;
    gameobject = gameobj;
    physicsMesh = physMesh;
    SpatialAccelerationStructure::Get().AddCollider(this, *gameobject->transformComponent);
}

void SpatialAccelerationStructure::ColliderComponent::Destroy() {
    physicsMesh = nullptr;
}

std::shared_ptr<GameObject>& SpatialAccelerationStructure::ColliderComponent::GetGameObject() {
    return ComponentRegistry::GAMEOBJECTS[gameobject];
}

// TODO: collider AABBs should be augmented to contain their motion over the next time increment.
    // If we ever use a second SAS for accelerating visibility queries too, then don't do it for that
void SpatialAccelerationStructure::ColliderComponent::RecalculateAABB(const TransformComponent& colliderTransform) {
    // std::cout << "Reacalculating AABB of " << this << "\n";
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
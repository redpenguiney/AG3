#pragma once
#include "spatial_acceleration_structure.hpp"

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

void SpatialAccelerationStructure::AddCollider(ColliderComponent& collider) {
    

}

void SpatialAccelerationStructure::RemoveCollider() {

}

void SpatialAccelerationStructure::UpdateCollider() {

}

SpatialAccelerationStructure::SpatialAccelerationStructure() {
    root = SasNode();
}

SpatialAccelerationStructure::~SpatialAccelerationStructure() {

}

SpatialAccelerationStructure::ColliderComponent* SpatialAccelerationStructure::ColliderComponent::New(TransformComponent* transformComponent) {
    auto ptr = COLLIDER_COMPONENTS.GetNew();
    ptr->transform = transformComponent;
    ptr->aabbType = AABBBoundingCube;
    SpatialAccelerationStructure::Get().AddCollider(*ptr);
    return ptr; 
}

void SpatialAccelerationStructure::ColliderComponent::Destroy() {
    COLLIDER_COMPONENTS.ReturnObject(this);
}

AABB SpatialAccelerationStructure::ColliderComponent::GetAABB() {
    if (aabbType == AABBBoundingCube) {
        glm::dvec3 min = {-std::sqrt(0.75), -std::sqrt(0.75), -std::sqrt(0.75)};
        glm::dvec3 max = {std::sqrt(0.75), std::sqrt(0.75), std::sqrt(0.75)};
        min *= transform->scale() * AABB_FAT_FACTOR;
        max *= transform->scale() * AABB_FAT_FACTOR;
        min += transform->position();
        max += transform->position();
        return AABB(min, max);
    }
    else {
        assert(false);
    }
}
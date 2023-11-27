#include "spatial_acceleration_structure.hpp"
#include "../gameobjects/transform_component.cpp"

// (GJK is the name of the collision algorithm)
// This file is in charge of narrowphase collision detection, meaning it actually does the hard work of determining exactly whether (and if so, how) given pairs of objects are colliding.
#pragma once
#include "../../external_headers/GLM/vec3.hpp"
#include <optional>
#include <vector>

struct CollisionInfo {
    glm::vec3 collisionNormal;
    std::vector<glm::dvec3> hitPoints;
    double penetrationDepth;
};

// GJK collision algorithm. Determines whether the given thingies are colliding, and if they are, the return value will contain the collision info.
std::optional<CollisionInfo> GJK(
    const TransformComponent& transform1,
    const SpatialAccelerationStructure::ColliderComponent& collider1,
    const TransformComponent& transform2,
    const SpatialAccelerationStructure::ColliderComponent& collider2
);
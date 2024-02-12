#include "spatial_acceleration_structure.hpp"
#include "../gameobjects/transform_component.hpp"

// This file is in charge of narrowphase collision detection, meaning it actually does the (oh god very so very) hard work of determining exactly whether (and if so, how) given pairs of objects are colliding.
#pragma once
#include "../../external_headers/GLM/vec3.hpp"
#include <optional>
#include <vector>

struct CollisionInfo {
    glm::dvec3 collisionNormal;

    // Contact points are in world space, will all be coplanar.
    // Pairs are <position, penetrationDepth>
    std::vector<std::pair<glm::dvec3, double>> contactPoints;


};

// GJK+EPA collision algorithms. Determines whether the given thingies are colliding, and if they are, the return value will contain the collision info.
std::optional<CollisionInfo> IsColliding(
    const TransformComponent& transform1,
    const SpatialAccelerationStructure::ColliderComponent& collider1,
    const TransformComponent& transform2,
    const SpatialAccelerationStructure::ColliderComponent& collider2
);
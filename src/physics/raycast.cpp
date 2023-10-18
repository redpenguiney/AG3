#pragma once
#include "raycast.hpp"
#include "spatial_acceleration_structure.hpp"

// copy pasted from https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm#C++_implementation
// If the ray intersects the triangle, sets intersection point and returns true, else returns false
bool IsTriangleColliding(glm::dvec3 origin, glm::dvec3 direction, glm::dvec3 triVertex0, glm::dvec3 triVertex1, glm::dvec3 triVertex2, glm::dvec3& intersectionPoint) {
    const double EPSILON = 0.0000001;
    glm::dvec3 edge1, edge2, h, s, q;
    double a, f, u, v;
    edge1 = triVertex1 - triVertex0;
    edge2 = triVertex2 - triVertex0;
    h = glm::cross(direction, edge2);
    a = glm::dot(edge1, h);

    if (a > -EPSILON && a < EPSILON)
        return false;    // This ray is parallel to this triangle.

    f = 1.0 / a;
    s = origin - triVertex0;
    u = f * glm::dot(s, h);

    if (u < 0.0 || u > 1.0)
        return false;

    q = glm::cross(s, edge1);
    v = f * glm::dot(direction, q);

    if (v < 0.0 || u + v > 1.0)
        return false;

    // At this stage we can compute t to find out where the intersection point is on the line.
    double t = f * glm::dot(edge2, q);

    if (t > EPSILON) // ray intersection
    {
        intersectionPoint = origin + direction * t;
        return true;
    }
    else {// This means that there is a line intersection but not a ray intersection.
        return false;
    }
}

// TODO: this uses objects' Mesh instead of a simplified thing 
// Returns first gameobject ray hits
std::shared_ptr<GameObject> Raycast(glm::dvec3 origin, glm::dvec3 direction) {
    auto possible_colliding = SpatialAccelerationStructure::Get().Query(origin, direction);

    for (auto & comp: possible_colliding) {
        auto& obj = comp->GetGameObject();
        Mesh::Get(obj->renderComponent->meshId).
    }
}
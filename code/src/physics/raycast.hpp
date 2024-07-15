#pragma once
#include "gameobjects/component_registry.hpp"
#include <glm/vec3.hpp>
#include <memory>

struct RaycastResult {
    glm::dvec3 hitPoint;
    glm::dvec3 hitNormal;
    std::shared_ptr<GameObject> hitObject;
};

// If ray did not hit anything, result.hitObject == nullptr.
RaycastResult Raycast(glm::dvec3 origin, glm::dvec3 direction);


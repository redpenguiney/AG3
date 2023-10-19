#pragma once
#include "../gameobjects/gameobject.hpp"
#include "../../external_headers/GLM/vec3.hpp"
#include <memory>

struct RaycastResult {
    glm::dvec3 hitPoint;
    glm::dvec3 hitNormal;
    std::shared_ptr<GameObject> hitObject;
};

RaycastResult Raycast(glm::dvec3 origin, glm::dvec3 direction);


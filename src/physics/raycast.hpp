#pragma once
#include "../gameobjects/gameobject.hpp"
#include "../../external_headers/GLM/vec3.hpp"
#include <memory>

std::shared_ptr<GameObject> Raycast(glm::dvec3 origin, glm::dvec3 direction);


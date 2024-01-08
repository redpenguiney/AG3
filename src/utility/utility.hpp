#pragma once

#include "../../external_headers/GLM/vec3.hpp"
#include "../../external_headers/GLM/vec4.hpp"

// uses radians
glm::dvec3 LookVector(double pitch, double yaw);

// places a little cube of the requested color at the requested position
// TODO: probably shouldn't use the rainbow cube for that
void DebugPlacePointOnPosition(glm::dvec3 position, glm::vec4 color = glm::vec4(1, 1, 1, 1));

// returns time in seconds
double Time();
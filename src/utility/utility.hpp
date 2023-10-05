#pragma once

#include "../../external_headers/GLM/ext.hpp"
#include <cmath>
// uses radians
inline glm::dvec3 LookVector(double pitch, double yaw) {
    return glm::dvec3(
        -sin(yaw),
        sin(pitch),
        cos(yaw)
    );
}
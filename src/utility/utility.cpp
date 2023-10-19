#include <cmath>
#include <cstdio>
#include "utility.hpp"

glm::dvec3 LookVector(double pitch, double yaw) {
    return glm::dvec3(
        sin(yaw) * cos(pitch),
        -sin(pitch),
        -cos(yaw) * cos(pitch)
        
    );
}
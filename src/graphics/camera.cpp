#include "camera.hpp"
#include <cassert>
#include <iostream>

Camera::Camera() {
    fieldOfView = 70;
}

// returns the camera's projection matrix.
// aspect is width/height
glm::mat4x4 Camera::GetProj(float aspect, float near, float far) {
    assert(aspect > 0);
    assert(near > 0);
    assert(far > near);
    return glm::perspective(fieldOfView, aspect, near, far);
}

glm::mat4x4 Camera::GetCamera() {
    return glm::identity<glm::mat4x4>();
}
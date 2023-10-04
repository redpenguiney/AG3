#include "camera.hpp"

Camera::Camera() {
    fieldOfView = 70;
}

// returns the camera's projection matrix.
// aspect is width/height
glm::mat4x4 Camera::GetProj(float aspect, float near, float far) {
    return glm::perspective(fieldOfView, aspect, near, far);
}

glm::mat4x4 Camera::GetCamera() {
    return glm::identity<glm::mat4x4>();
}
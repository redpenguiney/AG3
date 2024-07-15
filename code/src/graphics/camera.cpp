#include "camera.hpp"
#include <cassert>
#include <iostream>
#include "../debug/log.hpp"
#include "glm/gtx/string_cast.hpp"

Camera::Camera() {
    fieldOfView = 70;
}


glm::mat4x4 Camera::GetProj(float aspect) {
    assert(aspect > 0);
    assert(near > 0);
    assert(far > near);
    return glm::perspective(fieldOfView, aspect, near, far);
}

glm::mat4x4 Camera::GetCamera() {
    return glm::mat4x4(rotation);
}

glm::vec3 Camera::ProjectToScreen(glm::dvec3 point, float aspect) {
    glm::vec3 relPos = point - position;
    glm::vec4 clipSpacePoint = GetProj(aspect) * GetCamera() * glm::vec4(relPos.x, relPos.y, relPos.z, 1.0);
    return (1.0f + glm::vec3(clipSpacePoint/clipSpacePoint.w))/2.0f;
}
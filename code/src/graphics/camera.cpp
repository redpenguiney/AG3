#include "camera.hpp"
#include "debug/assert.hpp"
#include <iostream>
#include "../debug/log.hpp"
#include "glm/gtx/string_cast.hpp"

Camera::Camera() {
    fieldOfView = 70;
}


glm::mat4x4 Camera::GetProj(float aspect) {
    Assert(aspect > 0);
    Assert(near > 0);
    Assert(far > near);
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

glm::vec3 Camera::ProjectToWorld(glm::vec2 screenPos, glm::ivec2 windowSize)
{
    glm::vec4 clipSpacePos((screenPos / glm::vec2(windowSize)) * 2.0f - 1.0f, 0.0f, 1.0f);
    clipSpacePos.y =  -clipSpacePos.y;

    DebugLogInfo("Clip space = ", clipSpacePos);

    glm::vec3 worldSpacePos = glm::vec3(glm::inverse(GetCamera()) * glm::inverse(GetProj(windowSize.x / windowSize.y)) * clipSpacePos);
    return worldSpacePos;//glm::normalize(position - glm::dvec3(worldSpacePos));
}

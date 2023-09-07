#include "../../external_headers/GLM/ext.hpp"

class Camera {
    public: 
    float fieldOfView;

    Camera() {
        fieldOfView = 70;
    }

    // returns the camera's projection matrix.
    // aspect is width/height
    glm::mat4x4 GetProj(float aspect, float near=0.1, float far=1000.0) {
        return glm::perspective(fieldOfView, aspect, near, far);
    }

    glm::mat4x4 GetCamera() {
        return glm::translate(glm::identity<glm::mat4x4>(), glm::vec3(0.0, 0.0, -5.0)); //glm::identity<glm::mat4x4>();
    }
};
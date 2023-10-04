#include "../../external_headers/GLM/ext.hpp"

class Camera {
    public: 
    float fieldOfView;
    glm::dvec3 position;
    glm::quat rotation;

    Camera();

    // returns the camera's projection matrix.
    // aspect is width/height
    glm::mat4x4 GetProj(float aspect, float near=0.1, float far=1000.0);

    glm::mat4x4 GetCamera();
};
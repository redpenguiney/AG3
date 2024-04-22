#include "../../external_headers/GLM/ext.hpp"

class Camera {
    public: 
    float fieldOfView;
    float near = 0.1f;
    float far = 16384.0f;
    glm::dvec3 position;
    glm::quat rotation;

    Camera();

    // returns the camera's projection matrix.
    // aspect is window width/height
    glm::mat4x4 GetProj(float aspect);

    glm::mat4x4 GetCamera();

    // Takes a point in world space and converts it into clip/screen space (the range [-1, 1]).
    // Used for stuff like making a health bar gui appear over an enemy's head
    // aspect is window width/height.
    glm::vec2 ProjectToScreen(glm::dvec3, float aspect);

    // Takes a point in screen/clip space (the range [-1, 1]) and returns a normal vector facing into the screen.
    // aspect is window width/height.
    glm::vec3 ProjectToWorld(glm::vec2, float aspect);
};
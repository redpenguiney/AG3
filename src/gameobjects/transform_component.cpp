#include "../../external_headers/GLM/ext.hpp"

class TransformComponent {
    public:
    glm::dvec3 position;

    const glm::quat& rotation() const { return rotation_; } // allows public read only access to rotation
    const glm::dvec3& scale() const { return scale_; } // allows public read only access to scale

    private:
    // rotation and scale part of matrix will not neccesarily change every frame like position will due to floating origin
    // therefore we store it here to avoid matrix multiplcation/math
    glm::mat4x4 rotScaleMatrix;

    glm::quat rotation_;
    glm::dvec3 scale_; 
};
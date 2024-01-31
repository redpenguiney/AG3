#include "transform_component.hpp"

const glm::dvec3& TransformComponent::position() const { return position_; }
const glm::quat& TransformComponent::rotation() const { return rotation_; } 
const glm::vec3& TransformComponent::scale() const { return scale_; }

void TransformComponent::Init() {
    rotation_ = glm::identity<glm::quat>();
    scale_ = {1, 1, 1};
    position_ = {0, 0, 0};
    rotScaleMatrix = glm::identity<glm::mat4x4>();
    normalMatrix = glm::identity<glm::mat3x3>();
}

void TransformComponent::Destroy() {}

void TransformComponent::SetPos(glm::dvec3 pos) {
    position_ = pos;
    moved = true;
}

void TransformComponent::SetRot(glm::quat rot) {
    rotation_ = rot;
    moved = true;
    UpdateRotScaleMatrix();
}

void TransformComponent::SetScl(glm::vec3 scl) {
        scale_ = scl;
        moved = true;
        UpdateRotScaleMatrix();
    }

const glm::mat4x4& TransformComponent::GetGraphicsModelMatrix(const glm::dvec3 & cameraPosition) {
    rotScaleMatrix[3] = glm::vec4((position() - cameraPosition), 1.0);
    //mat = glm::translate(mat, glm::vec3(position - cameraPosition));
    // std::printf("\nPos is\n%f,%f,%f,%f,\n%f,%f,%f,%f,\n%f,%f,%f,%f,\n%f,%f,%f,%f,", mat[0][0], mat[0][1], mat[0][2], mat[0][3], mat[1][0], mat[1][1], mat[1][2], mat[1][3], mat[2][0], mat[2][1], mat[2][2], mat[2][3], mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
    return rotScaleMatrix;
}

// TODO: might be slow idk, store it if you have to
glm::dmat4x4 TransformComponent::GetPhysicsModelMatrix() const {
    glm::dmat4x4 mat = rotScaleMatrix;
    mat[3] = glm::dvec4(position_, 1);
    return mat;
}

    const glm::mat3& TransformComponent::GetNormalMatrix() const {
    return normalMatrix;
}

void TransformComponent::UpdateRotScaleMatrix() {
    // normalMatrix = glm::toMat3(rotation());
    rotScaleMatrix = glm::mat4x4(rotation()) * glm::scale(glm::identity<glm::mat4x4>(), (glm::vec3)scale());
    normalMatrix = glm::inverseTranspose(glm::mat3x3(rotScaleMatrix));
}
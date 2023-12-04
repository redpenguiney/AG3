#pragma once
#include "base_component.hpp"
#include "../../external_headers/GLM/ext.hpp"
#include "../../external_headers/GLM/gtx/quaternion.hpp"
#include <cstdio>

// TODO: header

// TODO: use doubles in matrices for physics???
class TransformComponent: public BaseComponent<TransformComponent> {
    public:

    const glm::dvec3& position() const { return position_; } // allows public read only access to rotation
    const glm::quat& rotation() const { return rotation_; } // allows public read only access to rotation
    const glm::vec3& scale() const { return scale_; } // allows public read only access to scale

    // Called when a gameobject is given this component.
    void Init() {
        rotation_ = glm::identity<glm::quat>();
        scale_ = {1, 1, 1};
        position_ = {0, 0, 0};
        rotScaleMatrix = glm::identity<glm::mat4x4>();
        normalMatrix = glm::identity<glm::mat3x3>();
    }

    // Called when this component is returned to a pool.
    void Destroy() {
        
    }

    void SetPos(glm::dvec3 pos) {
        position_ = pos;
        moved = true;
    }

    void SetRot(glm::quat rot) {
        rotation_ = rot;
        moved = true;
        UpdateRotScaleMatrix();
    }

    void SetScl(glm::vec3 scl) {
        scale_ = scl;
        moved = true;
        UpdateRotScaleMatrix();
    }

    // cameraPosition used for floating origin
    // TODO: option to not do floating origin
    const glm::mat4x4& GetGraphicsModelMatrix(const glm::dvec3 & cameraPosition) {
        rotScaleMatrix[3] = glm::vec4((position() - cameraPosition), 1.0);
        //mat = glm::translate(mat, glm::vec3(position - cameraPosition));
        // std::printf("\nPos is\n%f,%f,%f,%f,\n%f,%f,%f,%f,\n%f,%f,%f,%f,\n%f,%f,%f,%f,", mat[0][0], mat[0][1], mat[0][2], mat[0][3], mat[1][0], mat[1][1], mat[1][2], mat[1][3], mat[2][0], mat[2][1], mat[2][2], mat[2][3], mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
        return rotScaleMatrix;
    }

    // TODO: might be slow idk, store it if you have to
    glm::dmat4x4 GetPhysicsModelMatrix() const {
        glm::dmat4x4 mat = rotScaleMatrix;
        mat[3] = glm::dvec4(position_, 1);
        return mat;
    }

    const glm::mat3& GetNormalMatrix() const {
        return normalMatrix;
    }

    private:
    // after changing scale or rotation, we need to update the rot/scale matrix
    // we don't need to mess with position tho because its the only thing that touches the last column of the matrix and its set every frame anyways for floating origin
    // TODO: could instead of calling this when rot/scl changes, call this every frame if a rotSclChanged flag is set
    void UpdateRotScaleMatrix() {
        normalMatrix = glm::toMat3(rotation());
        rotScaleMatrix = glm::mat4x4(rotation()) * glm::scale(glm::identity<glm::mat4x4>(), (glm::vec3)scale());
    }

    // private constructor to enforce usage of object pool
    friend class ComponentPool<TransformComponent>;
    TransformComponent() {

    }

    // rotation and scale part of matrix will not neccesarily change every frame like position will due to floating origin
    // therefore we store it here to avoid matrix multiplcation/math
    glm::mat4x4 rotScaleMatrix;

    // normals shouldn't be scaled or rotated so they need their own matrix; TODO potential optimizations?
    glm::mat3 normalMatrix;

    // SAS needs to access moved variable
    friend class SpatialAccelerationStructure; 
    bool moved;
    glm::dvec3 position_;
    glm::quat rotation_;
    glm::vec3 scale_; 
};
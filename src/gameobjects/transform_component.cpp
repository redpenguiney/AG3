#pragma once
#include "component_pool.hpp"
#include "base_component.cpp"
#include "../../external_headers/GLM/ext.hpp"
#include <cstdio>

// TODO: use doubles in matrices for physics???
class TransformComponent: public BaseComponent {
    public:

    const glm::dvec3& position() const { return position_; } // allows public read only access to rotation
    const glm::quat& rotation() const { return rotation_; } // allows public read only access to rotation
    const glm::dvec3& scale() const { return scale_; } // allows public read only access to scale

    // DO NOT delete this pointer.
    static TransformComponent* New() {
        auto ptr = TRANSFORM_COMPONENTS.GetNew();
        ptr->rotation_ = glm::identity<glm::quat>();
        ptr->rotScaleMatrix = glm::identity<glm::mat4x4>();
        return ptr;
    }

    // call instead of deleting the pointer.
    // obviously don't touch component after this.
    void Destroy() {
        TRANSFORM_COMPONENTS.ReturnObject(this);
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

    void SetScl(glm::dvec3 scl) {
        scale_ = scl;
        moved = true;
        UpdateRotScaleMatrix();
    }

    // cameraPosition used for floating origin
    glm::mat4x4 GetModel(const glm::dvec3 & cameraPosition) {
        auto mat = rotScaleMatrix;
        mat[3] = glm::vec4((position() - cameraPosition), 1.0);
        //mat = glm::translate(mat, glm::vec3(position - cameraPosition));
        // std::printf("\nPos is\n%f,%f,%f,%f,\n%f,%f,%f,%f,\n%f,%f,%f,%f,\n%f,%f,%f,%f,", mat[0][0], mat[0][1], mat[0][2], mat[0][3], mat[1][0], mat[1][1], mat[1][2], mat[1][3], mat[2][0], mat[2][1], mat[2][2], mat[2][3], mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
        return mat;
    }

    private:
    // after changing scale or rotation, we need to update the rot/scale matrix
    // we don't need to mess with position tho because its the only thing that touches the last column of the matrix and its set every frame anyways for floating origin
    // TODO: could instead of calling this when rot/scl changes, call this every frame if a rotSclChanged flag is set
    void UpdateRotScaleMatrix() {
        rotScaleMatrix = glm::mat4x4(rotation()) * glm::scale(glm::identity<glm::mat4x4>(), (glm::vec3)scale());
    }

    // private constructor to enforce usage of object pool
    friend class ComponentPool<TransformComponent>;
    TransformComponent() {

    }

    friend class GraphicsEngine; // GE and PE both need to iterate through TRANSFORM_COMPONENTS
    // friend class PhysicsEngine;

    // object pool
    static inline ComponentPool<TransformComponent> TRANSFORM_COMPONENTS;

    // rotation and scale part of matrix will not neccesarily change every frame like position will due to floating origin
    // therefore we store it here to avoid matrix multiplcation/math
    glm::mat4x4 rotScaleMatrix;

    bool moved;
    glm::dvec3 position_;
    glm::quat rotation_;
    glm::dvec3 scale_; 
};
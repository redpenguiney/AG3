#pragma once
#include "component_pool.cpp"
#include "../../external_headers/GLM/ext.hpp"
#include <cstdio>

// TODO: use doubles in matrices for physics???
class TransformComponent {
    public:
    glm::dvec3 position;

    const glm::quat& rotation() const { return rotation_; } // allows public read only access to rotation
    const glm::dvec3& scale() const { return scale_; } // allows public read only access to scale

    static TransformComponent* New() {
        auto ptr = TRANSFORM_COMPONENTS.GetNew();
        ptr->rotScaleMatrix = glm::identity<glm::mat4x4>();
        return ptr;
    }

    // call instead of deleting the pointer.
    // obviously don't touch component after this.
    void Destroy() {
        TRANSFORM_COMPONENTS.ReturnObject(this);
    }

    // cameraPosition used for floating origin
    glm::mat4x4 GetModel(const glm::dvec3 & cameraPosition) {
        auto mat = rotScaleMatrix;
        mat[3] = glm::vec4((position - cameraPosition), 1.0);
        //mat = glm::translate(mat, glm::vec3(position - cameraPosition));
        // std::printf("\nPos is\n%f,%f,%f,%f,\n%f,%f,%f,%f,\n%f,%f,%f,%f,\n%f,%f,%f,%f,", mat[0][0], mat[0][1], mat[0][2], mat[0][3], mat[1][0], mat[1][1], mat[1][2], mat[1][3], mat[2][0], mat[2][1], mat[2][2], mat[2][3], mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
        return mat;
    }

    // this union exists so we can use a "free list", see component_pool.cpp
    bool live;
    union {
        // live state
        struct {
            
        };

        //dead state
        struct {
            TransformComponent* next; // pointer to next available component in pool
            unsigned int componentPoolId; // index into pools vector
        };
        
    };

    private:

    // private constructor to enforce usage of object pool
    friend class ComponentPool<TransformComponent, 65536>;
    TransformComponent() {

    }

    friend class GraphicsEngine; // GE and PE both need to iterate through TRANSFORM_COMPONENTS
    // friend class PhysicsEngine;

    // object pool
    static inline ComponentPool<TransformComponent, 65536> TRANSFORM_COMPONENTS;

    // rotation and scale part of matrix will not neccesarily change every frame like position will due to floating origin
    // therefore we store it here to avoid matrix multiplcation/math
    glm::mat4x4 rotScaleMatrix;

    glm::quat rotation_;
    glm::dvec3 scale_; 
};
#pragma once
#include "component_pool.cpp"
#include "../../external_headers/GLM/ext.hpp"

class TransformComponent {
    public:
    glm::dvec3 position;

    const glm::quat& rotation() const { return rotation_; } // allows public read only access to rotation
    const glm::dvec3& scale() const { return scale_; } // allows public read only access to scale

    static TransformComponent* New() {
        return TRANSFORM_COMPONENTS.GetNew();
    }

    // call instead of deleting the pointer.
    // obviously don't touch component after this.
    void Destroy() {
        TRANSFORM_COMPONENTS.ReturnObject(this);
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
    friend class ComponentPool<TransformComponent>;
    TransformComponent() {

    }

    friend class GraphicsEngine; // GE and PE both need to iterate through TRANSFORM_COMPONENTS
    // friend class PhysicsEngine;

    // object pool
    static ComponentPool<TransformComponent> TRANSFORM_COMPONENTS;

    // rotation and scale part of matrix will not neccesarily change every frame like position will due to floating origin
    // therefore we store it here to avoid matrix multiplcation/math
    glm::mat4x4 rotScaleMatrix;

    glm::quat rotation_;
    glm::dvec3 scale_; 
};
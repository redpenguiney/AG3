#pragma once
#include "base_component.hpp"
#include "../../external_headers/GLM/ext.hpp"
#include "../../external_headers/GLM/gtx/quaternion.hpp"
#include <cstdio>

// TODO: header

// TODO: use doubles in matrices for physics???
class TransformComponent: public BaseComponent<TransformComponent> {
    public:

    const glm::dvec3& position() const; // allows public read only access to rotation
    const glm::quat& rotation() const;// allows public read only access to rotation
    const glm::vec3& scale() const; // allows public read only access to scale

    // Called when a gameobject is given this component.
    void Init();

    // Called when this component is returned to a pool.
    void Destroy();

    void SetPos(glm::dvec3 pos);

    void SetRot(glm::quat rot);
    void SetScl(glm::vec3 scl);

    const glm::mat4x4& GetRotSclPhysicsMatrix() const {
        return rotScaleMatrix;
    }

    // cameraPosition used for floating origin
    // TODO: option to not do floating origin
    const glm::mat4x4& GetGraphicsModelMatrix(const glm::dvec3 & cameraPosition);
    
    glm::dmat4x4 GetPhysicsModelMatrix() const;

    const glm::mat3& GetNormalMatrix() const;

    private:
    // after changing scale or rotation, we need to update the rot/scale matrix
    // we don't need to mess with position tho because its the only thing that touches the last column of the matrix and its set every frame anyways for floating origin
    // TODO: could instead of calling this when rot/scl changes, call this every frame if a rotSclChanged flag is set
    void UpdateRotScaleMatrix();

    // private constructor to enforce usage of object pool
    friend class ComponentPool<TransformComponent>;

    // trivial constructor
    // TransformComponent();

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
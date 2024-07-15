#pragma once
#include "base_component.hpp"
#include "glm/ext.hpp"
#include "glm/gtx/quaternion.hpp"
#include <vector>

// TODO: use doubles in matrices for physics???
// TODO: split off the matrices into their own component to enable graphics optimization (by reducing memory needing to be retrieved each frame in UpdateRenderComponents()).
// Transform components store position/rotation/scale, they use a transform hierarchy (know that using it excessively carries a performance cost)
class TransformComponent: public BaseComponent<TransformComponent> {
    public:
    

    const glm::dvec3& Position() const; // Returns global (in world space) position of the object.
    const glm::quat& Rotation() const;// Returns global (in world space) rotation of the object. 
    const glm::vec3& Scale() const; // Returns global (in world space) scale of the object.

    // Called when a gameobject is given this component.
    void Init();

    // Called when this component is returned to a pool.
    void Destroy();

    // Sets position in WORLD space, regardless of the transform's parent, and affects the transform's children appropriately.
    void SetPos(glm::dvec3 pos);

    // Sets rotation in WORLD space, regardless of the transform's parent, and affects the transform's children appropriately.
    void SetRot(glm::quat rot);

    // Sets scale in WORLD space, regardless of the transform's parent, and affects the transform's children appropriately.
    void SetScl(glm::vec3 scl);

    

    const glm::mat4x4& GetRotSclPhysicsMatrix() const {
        return rotScaleMatrix;
    }

    // cameraPosition used for floating origin
    // TODO: option to not do floating origin
    const glm::mat4x4& GetGraphicsModelMatrix(const glm::dvec3 & cameraPosition);
    
    glm::dmat4x4 GetPhysicsModelMatrix() const;

    const glm::mat3& GetNormalMatrix() const;

    
    void SetParent(TransformComponent& newParent);

    // Returns the parent. Don't hold onto this pointer, as when the transform component gets deleted you're in trouble.
    TransformComponent* GetParent();

    private:
    // after changing scale or rotation, we need to update the rot/scale matrix
    // we don't need to mess with position tho because its the only thing that touches the last column of the matrix and its set every frame anyways for floating origin
    // TODO: could instead of calling this when rot/scl changes, call this every frame if a rotSclChanged flag is set
    void UpdateRotScaleMatrix();

    // private constructor to enforce usage of object pool
    friend class ComponentPool<TransformComponent>;

    // nullptr if no parent
    // Determines the transform that this transform inherits its position/rot/scl from
    TransformComponent* parent;

    // We have to store this to properly reset parents. (TODO: this doesn't happen very much, so perhaps store outside the component to reduce memory footprint?)
    std::vector<TransformComponent*> children;

    // trivial constructor
    // TransformComponent();

    // rotation and scale part of matrix will not neccesarily change every frame like position will due to floating origin
    // therefore we store it here to avoid matrix multiplcation/math
    glm::mat4x4 rotScaleMatrix;

    // true if rot/scale (or that of its parent) has changed and needs to be recalculated
    // bool dirtyRotScale;

    // void MakeChildrenDirty();
    // void MakeChildrenMoved();

    // normals shouldn't be scaled or rotated so they need their own matrix; TODO potential optimizations?
    glm::mat3 normalMatrix;

    // SAS needs to access the variable "moved"
    friend class SpatialAccelerationStructure; 

    // used for physics/sas optimizations, set to true when it's been moved and then set to false after it recalculates AABBs
    bool moved;
    // position, rotation, and scale are all global/in world space.
    glm::dvec3 position;
    glm::quat rotation;
    glm::vec3 scale; 
};
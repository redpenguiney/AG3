#include "transform_component.hpp"
#include <iostream>
#include "glm/gtx/string_cast.hpp"
#include "debug/log.hpp"
#include "debug/assert.hpp"

const glm::dvec3& TransformComponent::Position() const {
    return position;
}

const glm::vec3& TransformComponent::Scale() const {
    return scale;
}


const glm::quat& TransformComponent::Rotation() const {
    return rotation;
}

TransformComponent::TransformComponent() {
    rotation = glm::identity<glm::quat>();
    scale = {1, 1, 1};
    position = {0, 0, 0};
    rotScaleMatrix = glm::identity<glm::mat4x4>();
    normalMatrix = glm::identity<glm::mat3x3>();

    parent = nullptr;
    children = {};
}

TransformComponent::~TransformComponent() {}

// void TransformComponent::MakeChildrenDirty() {
//     for (auto & c: children) {
//         c->dirtyRotScale = true;
//         c->moved = true;
//         c->MakeChildrenDirty();
//     }
// }

// void TransformComponent::MakeChildrenMoved() {
//     for (auto & c: children) {
//         c->moved = true;
//         c->MakeChildrenMoved();
//     }
// }

TransformComponent* TransformComponent::GetParent() {
    return parent;
}

void TransformComponent::SetParent(TransformComponent& newParent) {
    // remove this transform from its current parent's list of children
    if (parent != nullptr) {
        for (auto & c: parent->children) {
            if (c == this) {
                c = parent->children.back();
                parent->children.pop_back();
                break;
            }
        }
    }

    parent = &newParent;

    // add this transform to its new parent's list of children
    parent->children.push_back(this);
}

void TransformComponent::SetPos(glm::dvec3 pos) {
    auto deltaPosition = pos - Position();
    for (auto & c: children) {
        c->SetPos(c->Position() + deltaPosition); // todo : delta position always gonna be the same as it recursively goes down the hierarchy, could optimize this?
    }
    position = pos;
    moved = true;
    
}

void TransformComponent::SetRot(glm::quat rot) {
    Assert(glm::length(rot)  > 0.001);
    rot = glm::normalize(rot);

    // // fun fact: quaternions can go up to 720 degrees, but the physics engine has a stroke when it goes past 360 (idk why)
    // // this clamps it  [0, 360] degrees
    // if (rot.x < 0) {
    //     // DebugLogInfo("Flipping quaternion.");
        
    //     rot.x *= -1;
    // }

    if (children.size() > 0) {
        auto deltaRotation = rot * glm::inverse(rotation);
         
        for (auto & c: children) {
            auto newRotation = c->Rotation() * deltaRotation;

            // calculate child's local position relative to us (the parent)
            glm::vec3 localP = glm::inverse(c->Rotation()) * glm::vec3(c->Position() - Position());

            // then rotate that by its new rotation to find its new relative position in world space
            c->SetPos(Position() + glm::dvec3(newRotation * localP));

            // then, actually apply the rotation.
            c->SetRot(newRotation);

        }
    }
    
    
    rotation = rot;
    UpdateRotScaleMatrix();
    // dirtyRotScale = true;
    moved = true;
    
}

void TransformComponent::SetScl(glm::vec3 scl) {
    //Assert(scl.x != 0 && scl.y != 0 && scl.z);
    if (scl.x <= 0) scl.x = 0.0001;
    if (scl.y <= 0) scl.y = 0.0001;
    if (scl.z <= 0) scl.z = 0.0001;

    if (children.size() > 0) {
        auto deltaScale = scl / scale;
         
        for (auto & c: children) {
            auto newScale = c->Scale() * deltaScale;

            // calculate child's local position relative to us (the parent)
            glm::vec3 localP = glm::inverse(c->Rotation()) * glm::vec3(c->Position() - Position());

            // then scale that its relative position by the new scale to get the real global position
            c->SetPos(Position() + glm::dvec3(c->Rotation() * (localP * deltaScale)));

            // then, actually apply the scaling.
            c->SetScl(newScale);

        }
    }
    
    
    scale = scl;
    UpdateRotScaleMatrix();
    // dirtyRotScale = true;
    moved = true;
}

const glm::mat4x4& TransformComponent::GetGraphicsModelMatrix(const glm::dvec3 & cameraPosition) {
    rotScaleMatrix[3] = glm::vec4((Position() - cameraPosition), 1.0);
    //mat = glm::translate(mat, glm::vec3(position - cameraPosition));
    // std::printf("\nPos is\n%f,%f,%f,%f,\n%f,%f,%f,%f,\n%f,%f,%f,%f,\n%f,%f,%f,%f,", mat[0][0], mat[0][1], mat[0][2], mat[0][3], mat[1][0], mat[1][1], mat[1][2], mat[1][3], mat[2][0], mat[2][1], mat[2][2], mat[2][3], mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
    return rotScaleMatrix;
}

// TODO: might be slow idk, store it if you have to
glm::dmat4x4 TransformComponent::GetPhysicsModelMatrix() const {
    glm::dmat4x4 mat = rotScaleMatrix;
    mat[3] = glm::dvec4(Position(), 1);
    return mat;
}

const glm::mat3& TransformComponent::GetNormalMatrix() const {
    return normalMatrix;
}

void TransformComponent::UpdateRotScaleMatrix() {
    // normalMatrix = glm::toMat3(rotation);
    rotScaleMatrix = glm::mat4x4(rotation) * glm::scale(glm::identity<glm::mat4x4>(), (glm::vec3)scale);
    Assert(!std::isnan(rotation[0]));
    normalMatrix = glm::inverseTranspose(glm::mat3x3(rotScaleMatrix));
    Assert(!std::isnan(normalMatrix[0][0]));
}
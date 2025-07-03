#pragma once
#include <string>
#include <vector>
#include <optional>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>

class Bone {
    public:
    std::string name; // for debug purposes
    unsigned int id; // what is actually used by shaders/etc, as well as the index of this bone in its mesh.
    
    std::vector<unsigned int> childrenBoneIndices; // indices into the mesh's bone vector
    int parentIndex; // -1 if not there

    // mesh space to bone space (in bind pose)
    glm::mat4x4 inverseBindTransform;

    // where the bone is in the bind pose.
    glm::mat4x4 baseBonePosition;
    
};

struct BoneKeyframe {
    //unsigned int boneIndex;
    glm::vec3 translation;
    glm::vec3 scale;
    glm::quat rotation;

    float timestamp;
    
};

//struct AnimationKeyframe {
//    float timestamp; // in seconds from start of animation
//    
//    std::vector<BoneKeyframe> boneKeyframes;
//
//};

// Each Animation has a BoneAnimation for every bone it affects.
struct BoneAnimation {
    std::vector<BoneKeyframe> keyframes; // sorted from start to end. 
    unsigned int boneIndex; // index of the bone this animation affects
};

// Max 2^15 bones per animation.
// Actual playing of animations is carried out by the graphics engine.
class Animation {
    public:
    std::string name;
    float duration;
    float priority; // higher number = higher priority; can be negative

    std::vector<BoneAnimation> boneAnimations; // one for every bone affected by the animation. In no particular order.

    // interpolates between keyframes.
    // this transform is relative to the bone's parent.
    // if the animation does not have a bone corresponding to this id, returns nullopt.
    std::optional<glm::mat4x4> BoneTransformAtTime(unsigned int boneId, float time) const;
};
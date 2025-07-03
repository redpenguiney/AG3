#include "animation.hpp"
#include "debug/log.hpp"
#include "debug/assert.hpp"
#include "glm/gtx/quaternion.hpp"

std::optional<glm::mat4x4> Animation::BoneTransformAtTime(unsigned int boneId, float time) const {

    // find the bone in boneAnimations
    const BoneAnimation* boneAnim = nullptr;
    for (auto& b : boneAnimations) {
        if (b.boneIndex == boneId) {
            boneAnim = &b;
            break;
        }
    }

    if (!boneAnim) { // then bone isn't affected by this animation: return nullopt
        return std::nullopt;
    }

   
    if (boneAnim->keyframes.size() == 1) { // then we can't interpolate (only one keyframe), just return that keyframe
        auto localMat = glm::translate(glm::identity<glm::mat4x4>(), boneAnim->keyframes[0].translation)
            * glm::toMat4(boneAnim->keyframes[0].rotation)
            * glm::scale(glm::identity<glm::mat4x4>(), boneAnim->keyframes[0].scale);

        return localMat;
    }

    // find the two keyframes closest to the requested time and return them
    for (unsigned int i = 0; i < boneAnim->keyframes.size(); i++) {
        const auto & keyframe = boneAnim->keyframes[i];
        if (keyframe.timestamp > time) { // then this is the next keyframe we're going to reach.

            Assert(i != 0); // we need to use the keyframe at i-1 so we can interpolate.

            const auto & prevKeyframe = boneAnim->keyframes[i - 1];

            float interpolationFactor = (time - prevKeyframe.timestamp) / (keyframe.timestamp - prevKeyframe.timestamp);
            glm::vec3 interpolatedPos = (1 - interpolationFactor) * prevKeyframe.translation + keyframe.translation * interpolationFactor;
            glm::vec3 interpolatedScl = (1 - interpolationFactor) * prevKeyframe.scale + keyframe.scale * interpolationFactor;
            glm::quat interpolatedRot = glm::slerp(prevKeyframe.rotation, keyframe.rotation, interpolationFactor);
            


            auto localMat = glm::translate(glm::identity<glm::mat4x4>(), interpolatedPos)
                * glm::toMat4(interpolatedRot)
                * glm::scale(glm::identity<glm::mat4x4>(), interpolatedScl);

            return localMat;
        }
    }
    
    DebugLogError("Bone does not have keyframe for time = ", time, ". Aborting.");
    abort();
}
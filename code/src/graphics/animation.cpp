#include "animation.hpp"
#include "debug/log.hpp"
#include "glm/gtx/quaternion.hpp"

std::optional<glm::mat4x4> Animation::BoneTransformAtTime(unsigned int boneId, float time) const {
    for (unsigned int i = 0; i < keyframes.size(); i++) {
        const auto & keyframe = keyframes[i];
        if (keyframe.timestamp > time) { // then this is the next keyframe we're going to reach.
            assert(i != 0); // we need to use the keyframe at i-1 so we can interpolate between.
            int boneIndex = -1;
            for (unsigned int ii = 0; ii < keyframe.boneKeyframes.size(); ii++) {
                if (keyframe.boneKeyframes[ii].boneIndex == boneId) {
                    boneIndex = ii;
                }
            }
            if (boneIndex == -1) { return std::nullopt; }

            const auto & prevKeyframe = keyframes[i - 1];

            float interpolationFactor = (time - prevKeyframe.timestamp) / (keyframe.timestamp - prevKeyframe.timestamp);
            glm::vec3 interpolatedPos = (1 - interpolationFactor) * prevKeyframe.boneKeyframes[boneIndex].translation + keyframe.boneKeyframes[boneIndex].translation * interpolationFactor;
            glm::vec3 interpolatedScl = (1 - interpolationFactor) * prevKeyframe.boneKeyframes[boneIndex].scale + keyframe.boneKeyframes[boneIndex].scale * interpolationFactor;
            glm::quat interpolatedRot = glm::slerp(prevKeyframe.boneKeyframes[boneIndex].rotation, keyframe.boneKeyframes[boneIndex].rotation, interpolationFactor);
            return 
                glm::translate(glm::identity<glm::mat4x4>(), interpolatedPos)
                * glm::toMat4(interpolatedRot)
                * glm::scale(glm::identity<glm::mat4x4>(), interpolatedScl)
            ;
        }
    }
    
    DebugLogError("Bone does not have keyframe for time = ", time, ". Aborting.");
    abort();
}
#include "animation_component.hpp"
#include "graphics/mesh.hpp"

void AnimationComponent::ClearBoneMatrix(unsigned boneId) {
    for (unsigned i = 0; i < overrides.size(); i++) {
        if (overrides[i].boneId == boneId) {
            overrides[i] = overrides.back();
            overrides.pop_back();
            return;
        }
    }
}



AnimationComponent::AnimationComponent(RenderComponent* comp):
    renderComponent(comp) ,
    mesh((Assert(comp != nullptr), Mesh::Get(comp->meshId)))
{
    // DebugLogInfo("INITIALIZING ANIM COMP");
    Assert(mesh->vertexFormat.supportsAnimation);

    currentlyPlaying = {};
}

bool AnimationComponent::IsPlaying(std::string animName) {
    for (auto & anim : currentlyPlaying) {
        if (anim.anim->name == animName && anim.playbackPosition < anim.anim->duration) {
            return true;
        }
    }
    return false;
}

void AnimationComponent::SetBoneBindSpaceTransformMatrix(unsigned boneId, glm::mat4x4 transform, float priority) {
    for (auto & override : overrides) {
        if (override.boneId == boneId) {
            override.offset = transform;
            override.priority = priority;
            return;
        }
    }
    overrides.push_back(BoneOverride{ .offset = transform, .priority = priority, .boneId = boneId });
}

void AnimationComponent::SetBoneBindSpaceTransformMatrix(std::string boneName, glm::mat4x4 transform, float priority) {
    for (auto& b : mesh->bones.value()) {
        if (b.name == boneName) {
            SetBoneBindSpaceTransformMatrix(b.id, transform, priority);
            return;
        }
    }
    DebugLogError("Couldn\'t find bone ", boneName);
    Assert(false); 
}

void AnimationComponent::PlayAnimation(std::string animName, bool looped) {
    Assert(!IsPlaying(animName));
    for (auto & anim: mesh->animations.value()) {
        if (anim.name == animName) {
            currentlyPlaying.push_back(PlayingAnimation {&anim, 0, looped});
        }
    }
}

void AnimationComponent::StopAnimation(std::string animName) {
    Assert(IsPlaying(animName));
    std::erase_if(currentlyPlaying, [&animName](const PlayingAnimation& a) {return a.anim->name == animName;});
}

AnimationComponent::~AnimationComponent() {
    renderComponent = nullptr;
    //mesh = nullptr;
    currentlyPlaying.clear();
}

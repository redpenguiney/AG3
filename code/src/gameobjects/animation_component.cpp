#include "animation_component.hpp"
#include "graphics/mesh.hpp"

void AnimationComponent::Init(RenderComponent* comp) {
    // DebugLogInfo("INITIALIZING ANIM COMP");
    Assert(comp != nullptr);
    mesh = Mesh::Get(comp->meshId);
    Assert(mesh->vertexFormat.supportsAnimation);
    renderComponent = comp;

    currentlyPlaying = {};
}

AnimationComponent::AnimationComponent() {}

bool AnimationComponent::IsPlaying(std::string animName) {
    for (auto & anim : currentlyPlaying) {
        if (anim.anim->name == animName && anim.playbackPosition < anim.anim->duration) {
            return true;
        }
    }
    return false;
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

void AnimationComponent::Destroy() {
    renderComponent = nullptr;
    mesh = nullptr;
    currentlyPlaying.clear();
}
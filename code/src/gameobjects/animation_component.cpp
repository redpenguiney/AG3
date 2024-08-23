#include "animation_component.hpp"
#include "graphics/mesh.hpp"

AnimationComponent::AnimationComponent(RenderComponent* comp): 
    renderComponent(comp) 
{
    // DebugLogInfo("INITIALIZING ANIM COMP");
    Assert(comp != nullptr);
    mesh = Mesh::Get(comp->meshId);
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
    mesh = nullptr;
    currentlyPlaying.clear();
}
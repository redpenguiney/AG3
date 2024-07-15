#pragma once
#include "gameobjects/base_component.hpp"
#include "gameobjects/render_component.hpp"

#include <string>
#include <memory>

class Mesh;
class Animation;
class GraphicsEngine;

// Use with a render component that uses a mesh that supports animation. 
class AnimationComponent: public BaseComponent<AnimationComponent> {
    public:

    // Called when a gameobject is given this component.
    void Init(RenderComponent*);

    // Called when this component is returned to a pool.
    void Destroy();

    void PlayAnimation(std::string animName, bool loop = false);
    void StopAnimation(std::string animName);
    bool IsPlaying(std::string animName);

    private:
    friend class GraphicsEngine;

    struct PlayingAnimation {
        const Animation* anim; // should always be valid
        float playbackPosition; // playback position in seconds, change as you please.
        bool looped;
    };

    std::vector<PlayingAnimation> currentlyPlaying;

    

    RenderComponent* renderComponent;
    std::shared_ptr<Mesh> mesh;

    //private constructor to enforce usage of object pool
    friend class ComponentPool<AnimationComponent>;
    AnimationComponent();
};



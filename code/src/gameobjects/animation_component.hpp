#pragma once
#include "gameobjects/base_component.hpp"
#include "gameobjects/render_component.hpp"

#include <string>
#include <memory>

class Mesh;
class Animation;
class GraphicsEngine;

// Use with a render component that uses a mesh that supports animation. 
class AnimationComponent : public BaseComponent {
public:

    // Called when a gameobject is given this component.
    //void Init(RenderComponent*);

    // Called when this component is returned to a pool.
    //void Destroy();

    void PlayAnimation(std::string animName, bool loop = false);
    void StopAnimation(std::string animName);
    bool IsPlaying(std::string animName);

    // Makes the given bone be transformed by the given matrix from its builtin/rig/T-pose position.
        // Priority controls whether animations will override this or not.
    void SetBoneBindSpaceTransformMatrix(unsigned boneId, glm::mat4x4 transform, float priority);
    void SetBoneBindSpaceTransformMatrix(std::string boneName, glm::mat4x4 transform, float priority);
    // Makes the given bone be transformed by the given matrix from the mesh origin.
        // Priority controls whether animations will override this or not.
    //void SetBoneModelSpaceTransformMatrix(unsigned boneId, glm::mat4x4 transform, int priority);

    // Undoes SetBoneSpaceTransformMatrix()/SetBoneModelSpaceTransformMatrix() for the given boneId.
    void ClearBoneMatrix(unsigned boneId);

    AnimationComponent(const AnimationComponent&) = delete;
    AnimationComponent(RenderComponent*);
    ~AnimationComponent();

private:
    friend class GraphicsEngine;

    struct PlayingAnimation {
        const Animation* anim; // should always be valid
        float playbackPosition; // playback position in seconds, change as you please.
        bool looped;
    };

    struct BoneOverride {
        glm::mat4x4 offset;
        float priority;
        unsigned boneId;
    };

    std::vector<PlayingAnimation> currentlyPlaying;

    std::vector<BoneOverride> overrides;

    const RenderComponent* renderComponent;
    std::shared_ptr<Mesh> mesh;

    //private constructor to enforce usage of object pool
    //friend class ComponentPool<AnimationComponent>;
    
};



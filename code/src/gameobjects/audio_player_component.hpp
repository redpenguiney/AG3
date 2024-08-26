#pragma once
#include "base_component.hpp"
#include "../audio/sound.hpp"
#include <memory>
#include <optional>
#include <glm/vec3.hpp>

class GameObject;

class AudioPlayerComponent: public BaseComponent {
public:
    
    // Called when a gameobject is given this component.
    //void Init(GameObject* object, std::optional<std::shared_ptr<Sound>> soundToUse = std::nullopt);

    // Called when this component is returned to a pool.
    //void Destroy();

    AudioPlayerComponent(const AudioPlayerComponent&) = delete;
    AudioPlayerComponent(GameObject* object, std::optional<std::shared_ptr<Sound>> soundToUse = std::nullopt);
    ~AudioPlayerComponent();

    GameObject* const object;

    // Makes the output of the sound depend on its position
    bool positional;

    // requires positional to be true, and requires the AudioPlayerComponent to be paired with a RigidBodyComponent. Does the doppler effect based on object velocity.
    bool doppler;

    float pitch;
    float volume;

    // How quickly the sound gets quieter with distance. Requires positional to be true.
    float rolloff;

    // distance at which the sound hits 0. Requires positional to be true.
    float maxDistance;

    bool looped;

    void Play(float startTime = 0); // plays the sound starting at the given number of seconds in 
    void Pause(); // stops playback of the sound
    void Resume(); // plays the sound at its current position
    void Stop();

    bool IsPlaying();

    // stops any sounds currently playing fyi
    void SetSound(std::shared_ptr<Sound>);

private:

    friend class AudioEngine;

    // DONT call this unless you're the audio engine, sets 
    //void Update(glm::dvec3 microphonePosition);

    // // When Play()/similar is called, the sound doesn't actually start playing until Update() is called, these 2 variables keep track of whether/where to start playing 
    // std::optional<bool> startPlaying; // if value and its true, will start playing, if value and its false, will stop playing
    // std::optional<float> startPlayingAt; // if value, then when Update() is called the sound will begin playing from this time 

    std::shared_ptr<Sound> sound;
    bool isPlaying;
    unsigned int audioSourceId;
};
#include "audio_player_component.hpp" 
#include "component_registry.hpp"
#include "../audio/checked_openal_call.cpp"
#include <cassert>

// TODO: can we just call the OAL functions as soon as it starts? might be bad for perf idk prob fine

void AudioPlayerComponent::Init(GameObject* gameObject, std::optional<std::shared_ptr<Sound>> soundToUse) {
    CheckedOpenALCall(alGenSources(1, &audioSourceId));

    assert(gameObject != nullptr);
    object = gameObject;

    if (soundToUse.has_value()) {
        assert(soundToUse.value() != nullptr);
        sound = *soundToUse;
        CheckedOpenALCall(alSourcei(audioSourceId, AL_BUFFER, sound->alAudioBufferId));
    }
    else {
        sound = nullptr;
    }

    positional = false;
    doppler = false;

    pitch = 0.5;
    volume = 0.5;

    rolloff = 1.0;
    maxDistance = 1.0;

    looped = false;
    isPlaying = false;
}

void AudioPlayerComponent::Destroy() {
    Stop();
    sound = nullptr;
    CheckedOpenALCall(alDeleteSources(1, &audioSourceId));
}

void AudioPlayerComponent::Update(glm::dvec3 microphonePosition) {
    if (sound != nullptr) {
        CheckedOpenALCall(alSourcef(audioSourceId, AL_PITCH, pitch));
        CheckedOpenALCall(alSourcef(audioSourceId, AL_GAIN, volume));
        CheckedOpenALCall(alSourcef(audioSourceId, AL_MAX_DISTANCE, maxDistance));
        CheckedOpenALCall(alSourcef(audioSourceId, AL_ROLLOFF_FACTOR, rolloff));

        if (positional) {
            assert(object->transformComponent);
            glm::vec3 relPos = object->transformComponent->Position() - microphonePosition;
            CheckedOpenALCall(alSourcefv(audioSourceId, AL_POSITION, (float*)&relPos));
            if (doppler) {
                assert(object->rigidbodyComponent);
                CheckedOpenALCall(alSourcefv(audioSourceId, AL_VELOCITY, (float*)&object->rigidbodyComponent->velocity));
            }
            
        }
        
        CheckedOpenALCall(alSourcei(audioSourceId, AL_LOOPING, looped));

        // sound uses floating origin too
        CheckedOpenALCall(alSourcei(audioSourceId, AL_SOURCE_RELATIVE, true));
    }
    
}

void AudioPlayerComponent::SetSound(std::shared_ptr<Sound> newSound) {
    if (sound != nullptr) {
        Stop();
    }
    sound = newSound;
    CheckedOpenALCall(alSourcei(audioSourceId, AL_BUFFER, sound->alAudioBufferId));
}

void AudioPlayerComponent::Play() {
    
    // if (!isPlaying) {
    //     startPlaying = true;
    //     isPlaying = true;
    // }

    // startPlayingAt = 0;

    Play(0);
}

void AudioPlayerComponent::Play(float startTime) {
    assert(sound != nullptr);
    CheckedOpenALCall(alSourcef(audioSourceId, AL_SEC_OFFSET, startTime));
    CheckedOpenALCall(alSourcePlay(audioSourceId));
}

void AudioPlayerComponent::Pause() {
    assert(sound != nullptr);
    // if (isPlaying) {
    //     startPlaying 
    // }

    CheckedOpenALCall(alSourcePause(audioSourceId));
}

void AudioPlayerComponent::Stop() {
    assert(sound != nullptr);
    CheckedOpenALCall(alSourceStop(audioSourceId));
}
 
bool AudioPlayerComponent::IsPlaying() {
    return isPlaying;
}

#include "audio_player_component.hpp" 
#include "gameobject.hpp"
#include "../audio/checked_openal_call.cpp"
#include "debug/assert.hpp"

// TODO: can we just call the OAL functions as soon as it starts? might be bad for perf idk prob fine

AudioPlayerComponent::AudioPlayerComponent(GameObject* gameObject, std::optional<std::shared_ptr<Sound>> soundToUse): object(gameObject) {
    CheckedOpenALCall(alGenSources(1, &audioSourceId));

    Assert(object != nullptr);

    if (soundToUse.has_value()) {
        Assert(soundToUse.value() != nullptr);
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

AudioPlayerComponent::~AudioPlayerComponent() {
    Stop();
    sound = nullptr;
    CheckedOpenALCall(alDeleteSources(1, &audioSourceId));
}

//void AudioPlayerComponent::Update(glm::dvec3 microphonePosition) {
//    if (sound != nullptr) {
//        CheckedOpenALCall(alSourcef(audioSourceId, AL_PITCH, pitch));
//        CheckedOpenALCall(alSourcef(audioSourceId, AL_GAIN, volume));
//        CheckedOpenALCall(alSourcef(audioSourceId, AL_MAX_DISTANCE, maxDistance));
//        CheckedOpenALCall(alSourcef(audioSourceId, AL_ROLLOFF_FACTOR, rolloff));
//
//        if (positional) {
//            Assert(object->transformComponent);
//            glm::vec3 relPos = object->Get<TransformComponent>().Position() - microphonePosition;
//            CheckedOpenALCall(alSourcefv(audioSourceId, AL_POSITION, (float*)&relPos));
//            if (doppler) {
//                Assert(object->rigidbodyComponent);
//                CheckedOpenALCall(alSourcefv(audioSourceId, AL_VELOCITY, (float*)&object->rigidbodyComponent->velocity));
//            }
//            
//        }
//        
//        CheckedOpenALCall(alSourcei(audioSourceId, AL_LOOPING, looped));
//
//        // sound uses floating origin too
//        CheckedOpenALCall(alSourcei(audioSourceId, AL_SOURCE_RELATIVE, true));
//    }
//    
//}

void AudioPlayerComponent::SetSound(std::shared_ptr<Sound> newSound) {
    if (sound != nullptr) {
        Stop();
    }
    sound = newSound;
    CheckedOpenALCall(alSourcei(audioSourceId, AL_BUFFER, sound->alAudioBufferId));
}

void AudioPlayerComponent::Play(float startTime) {
    Assert(sound != nullptr);
    CheckedOpenALCall(alSourcef(audioSourceId, AL_SEC_OFFSET, startTime));
    Resume();
}

void AudioPlayerComponent::Pause() {
    Assert(sound != nullptr);
    // if (isPlaying) {
    //     startPlaying 
    // }

    CheckedOpenALCall(alSourcePause(audioSourceId));
}

void AudioPlayerComponent::Resume() {
    Assert(sound != nullptr);

    CheckedOpenALCall(alSourcePlay(audioSourceId))
}

void AudioPlayerComponent::Stop() {
    Assert(sound != nullptr);
    CheckedOpenALCall(alSourceStop(audioSourceId));
}
 
bool AudioPlayerComponent::IsPlaying() {
    return isPlaying;
}

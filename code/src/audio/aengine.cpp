#include "aengine.hpp"
#include "../gameobjects/gameobject.hpp"
#include <vector>
#include <stdlib.h>

#ifdef IS_MODULE
AudioEngine* _AUDIO_ENGINE_ = nullptr;
void AudioEngine::SetModuleAudioEngine(AudioEngine* engine) {
    _AUDIO_ENGINE_ = engine;
}
#endif

AudioEngine& AudioEngine::Get() {
    #ifdef IS_MODULE
    Assert(_AUDIO_ENGINE_ != nullptr);
    return *_AUDIO_ENGINE_;
    #else
    static AudioEngine engine;
    return engine;
    #endif
}

// because msvc doesn't support setenv(), we have a little wrapper for them taken from https://stackoverflow.com/questions/17258029/c-setenv-undefined-identifier-in-visual-studio
#ifndef setenv
int setenv(const char* name, const char* value, int overwrite)
{
    int errcode = 0;
    if (!overwrite) {
        size_t envsize = 0;
        errcode = getenv_s(&envsize, NULL, 0, name);
        if (errcode || envsize) return errcode;
    }
    return _putenv_s(name, value);
}
#endif

AudioEngine::AudioEngine() {
    // Disable debug logging except in case of warnings/errors.
    //Assert(setenv("ALSOFT_LOGLEVEL", "0", 1) == 0);
    // doesn't seem to work, set the env variable manually before running

    microphonePosition = {0, 0, 0};

    device = alcOpenDevice(0); // 0 means use the default device.

    if (!device) {
        std::cout << "Fatal error when attempting to open audio device. Aborting.\n";
    }

    std::vector<ALCint> attributes = {ALC_MONO_SOURCES, 1 << 24, ALC_STEREO_SOURCES, 1 << 24};

    // for some reason, something called hrtf makes the audio sound bad so we try to turn it off if possible
    // see https://openal-soft.org/openal-extensions/SOFT_HRTF.txt and https://github.com/kcat/openal-soft/issues/825 
    if (alcIsExtensionPresent(device, "ALC_SOFT_HRTF") == AL_TRUE) {
        attributes.push_back(/*ALC_HRTF_SOFT*/ 0x1992);
        attributes.push_back(/*ALC_HRTF_DISABLED_SOFT*/ 0x0000);   
    }
    else {
        DebugLogError("WARNING: ALC_SOFT_HRTF was not found! Audio quality may be severely effected!");
    }

    attributes.push_back(0);    
    context = alcCreateContext(device, attributes.data());

    CheckedOpenALCall(alcMakeContextCurrent(context));
}

void AudioEngine::Update() {

    
    for (auto it = GameObject::SystemGetComponents<AudioPlayerComponent, TransformComponent, RigidbodyComponent>({ ComponentBitIndex::AudioPlayer });  it.Valid(); it++) {
        auto& [audioPlayerComponent, transformComponent, rigidbodyComponent] = *it;

        if (audioPlayerComponent->sound != nullptr) {
            CheckedOpenALCall(alSourcef(audioPlayerComponent->audioSourceId, AL_PITCH, audioPlayerComponent->pitch));
            CheckedOpenALCall(alSourcef(audioPlayerComponent->audioSourceId, AL_GAIN, audioPlayerComponent->volume));
            CheckedOpenALCall(alSourcef(audioPlayerComponent->audioSourceId, AL_MAX_DISTANCE, audioPlayerComponent->maxDistance));
            CheckedOpenALCall(alSourcef(audioPlayerComponent->audioSourceId, AL_ROLLOFF_FACTOR, audioPlayerComponent->rolloff));
            
            if (audioPlayerComponent->positional) {
                Assert(transformComponent != nullptr);
                glm::vec3 relPos = transformComponent->Position() - microphonePosition;
                CheckedOpenALCall(alSourcefv(audioPlayerComponent->audioSourceId, AL_POSITION, (float*)&relPos));
                if (audioPlayerComponent->doppler) {
                    Assert(rigidbodyComponent != nullptr);
                    CheckedOpenALCall(alSourcefv(audioPlayerComponent->audioSourceId, AL_VELOCITY, (float*)&rigidbodyComponent->velocity));
                }
                        
            }
                    
            CheckedOpenALCall(alSourcei(audioPlayerComponent->audioSourceId, AL_LOOPING, audioPlayerComponent->looped));
            
            // sound uses floating origin too
            CheckedOpenALCall(alSourcei(audioPlayerComponent->audioSourceId, AL_SOURCE_RELATIVE, true));
        }

    }
}

AudioEngine::~AudioEngine() {
    alcDestroyContext(context);
    alcCloseDevice(device);
}
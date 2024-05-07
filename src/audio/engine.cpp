#include "engine.hpp"
#include "../gameobjects/component_registry.hpp"
#include <vector>

AudioEngine& AudioEngine::Get() {
    static AudioEngine engine;
    return engine;
}

AudioEngine::AudioEngine() {
    

    microphonePosition = {0, 0, 0};

    device = alcOpenDevice(0); // 0 means use the default device.

    if (!device) {
        std::cout << "Fatal error when attempting to open audio device. Aborting.\n";
    }

    std::vector<ALCint> attributes = {ALC_MONO_SOURCES, 1 << 24, ALC_STEREO_SOURCES, 1 << 24};

    // for some reason, something called hrtf makes the audio sound so we try to turn it off if possible
    // see https://openal-soft.org/openal-extensions/SOFT_HRTF.txt and https://github.com/kcat/openal-soft/issues/825 
    if (alcIsExtensionPresent(device, "ALC_SOFT_HRTF") == AL_TRUE) {
        attributes.push_back(/*ALC_HRTF_SOFT*/ 0x1992);
        attributes.push_back(/*ALC_HRTF_DISABLED_SOFT*/ 0x0000);   
    }
    else {
        std::cout << "WARNING: ALC_SOFT_HRTF was not found! Audio quality may be severely effected!\n";
    }

    attributes.push_back(0);    
    context = alcCreateContext(device, attributes.data());

    CheckedOpenALCall(alcMakeContextCurrent(context));
}

void AudioEngine::Update() {


    for (auto & [audioPlayerComponent]: ComponentRegistry::Get().GetSystemComponents<AudioPlayerComponent>()) {
        audioPlayerComponent->Update(microphonePosition);
    }
}

AudioEngine::~AudioEngine() {
    alcDestroyContext(context);
    alcCloseDevice(device);
}
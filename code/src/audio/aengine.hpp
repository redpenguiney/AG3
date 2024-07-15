#pragma once
#include "checked_openal_call.cpp"
#include <GLM/vec3.hpp>

// This class is in charge of initializing OpenAL, (TODO: they may have to decide which sounds to play each frame when too many are playing???)
// It also goes through AudioPlayerComponents every frame when AudioEngine::Update() is called in order to set sound position, velocity, pitch, doppler effect, etc.

class AudioEngine {
    public:
        
        static AudioEngine& Get();

        // When modules (shared libraries) get their copy of this code, they need to use a special version of AudioEngine::Get().
        // This is so that both the module and the main executable have access to the same singleton. 
        // The executable will provide each shared_library with a pointer to the audio engine.
        #ifdef IS_MODULE
        static void SetModuleAudioEngine(AudioEngine* engine);
        #endif

        void Update();

        glm::dvec3 microphonePosition;

    private:
        ALCdevice* device;
        ALCcontext* context;
        
        AudioEngine();
        ~AudioEngine();
};
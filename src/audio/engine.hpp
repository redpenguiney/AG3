#pragma once
#include "checked_openal_call.cpp"
#include "../../external_headers/GLM/vec3.hpp"

// This class is in charge of initializing OpenAL, (TODO: they may have to decide which sounds to play each frame when too many are playing???)
// It also goes through AudioPlayerComponents every frame when AudioEngine::Update() is called in order to set sound position, velocity, pitch, doppler effect, etc.

class AudioEngine {
    public:
        
        static AudioEngine& Get();
        void Update();

        glm::dvec3 microphonePosition;

    private:
        ALCdevice* device;
        ALCcontext* context;
        
        AudioEngine();
        ~AudioEngine();
};
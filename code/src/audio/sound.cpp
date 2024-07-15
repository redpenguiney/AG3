#include "sound.hpp"
#include "checked_openal_call.cpp"
#include "../external_headers/wav_loader/wav_loader.h"
#include <cassert>
#include <memory>
#include "aengine.hpp"

// some wacky function i copypasted from https://stackoverflow.com/questions/8147027/how-do-i-call-stdmake-shared-on-a-class-with-only-protected-or-private-const .
// Lets me put use std::make_shared on stuff with private constructors
template <typename... Args>
inline std::shared_ptr<Sound> protected_make_shared( Args&&... args )
{
  struct helper : public Sound
  {
    helper( Args&&... args )
        : Sound(std::forward< Args >(args)... )
    {}
  };

  return std::make_shared< helper >( std::forward< Args >( args )... );
}

std::shared_ptr<Sound> Sound::New(std::string path) {
    return protected_make_shared(path);
}

Sound::Sound(std::string path) {
    AudioEngine::Get(); // this ensures that the audio engine / OpenAL is loaded before we do anything else.

    AudioFile<short> audioFile;
    bool success = audioFile.load(path);
    if (!success) {
        std::cout << "Failure to load audio file " << path << ". Aborting.\n";
        abort();
    }

    int sampleRate = audioFile.getSampleRate();
    int bitDepth = audioFile.getBitDepth();

    stereo = audioFile.isStereo();

    // find format of the audio file
    ALenum format; 
    if(!stereo && bitDepth == 8) {
        format = AL_FORMAT_MONO8;
    } else if(!stereo && bitDepth == 16) {
        format = AL_FORMAT_MONO16;
    } else if(stereo && bitDepth == 8){
        format = AL_FORMAT_STEREO8;
    } else if(stereo && bitDepth == 16) {
        format = AL_FORMAT_STEREO16;
    } else
    {
        std::cout << "ERROR: unrecognised sound format: " << bitDepth << " bit depth. Aborting.\n";
        abort();
    };

    CheckedOpenALCall(alGenBuffers(1, &alAudioBufferId));
    
    
    int size = audioFile.getNumChannels() * audioFile.getNumSamplesPerChannel() * 1;
    size -= size % 8; // must be multiple of 4 for alignment reasons or something
    CheckedOpenALCall(alBufferData(alAudioBufferId, format, &(audioFile.samples[0][0]), size, sampleRate));
}

Sound::~Sound() {
    CheckedOpenALCall(alDeleteBuffers(1, &alAudioBufferId));
}
#pragma once
#include <memory>
#include <string>

// A sound stores (or TODO: streams from a file) audio in a buffer. Then, any number of AudioPlayerComponents play the contents of that buffer.
class Sound {
    public:

    // TODO: path must be to a .wav file right now.
    static std::shared_ptr<Sound> New(std::string path);    

    protected:
    Sound(std::string path);
    ~Sound();

    private:
    
    friend class AudioPlayerComponent;

    unsigned int alAudioBufferId;

    // true if stereo, false if mono
    bool stereo;
};
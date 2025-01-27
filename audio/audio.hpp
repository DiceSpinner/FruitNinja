#ifndef AUDIO_H
#define AUDIO_H
#include <al/al.h>

void initALContext();
void destroyALContext();

class AudioClip {
private:
	ALuint audioBuffer = 0;
public:
	AudioClip(const char* path);
	ALuint Get() const;
	~AudioClip();
};
#endif
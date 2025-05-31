#ifndef AUDIO_CLIP_H
#define AUDIO_CLIP_H
#include <AL/al.h>

class AudioClip {
private:
	ALuint audioBuffer = 0;
public:
	AudioClip(const char* path);
	AudioClip(AudioClip&& other) noexcept;
	AudioClip(const AudioClip& other) = delete;
	AudioClip& operator = (AudioClip&& other) noexcept;
	AudioClip& operator = (const AudioClip& other) = delete;

	ALuint Get() const;
	~AudioClip();
};
#endif
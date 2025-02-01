#include <AL/al.h>

class AudioClip {
private:
	ALuint audioBuffer = 0;
public:
	AudioClip(const char* path);
	ALuint Get() const;
	~AudioClip();
};
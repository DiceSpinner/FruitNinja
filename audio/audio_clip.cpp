#include "audio_clip.hpp"
#include <AudioFile.h>

AudioClip::AudioClip(const char* path) {
	AudioFile<short> audioFile;
	audioFile.load("sounds/abyss.wav");

	ALuint soundBuffer;
	alGenBuffers(1, &soundBuffer);
	ALuint format;
	if (audioFile.isMono()) {
		if (audioFile.getBitDepth() == 8) {
			format = AL_FORMAT_MONO8;
		}
		else {
			format = AL_FORMAT_MONO16;
		}
	}
	else {
		if (audioFile.getBitDepth() == 8) {
			format = AL_FORMAT_STEREO8;
		}
		else {
			format = AL_FORMAT_STEREO16;
		}
	}
	auto numChannels = audioFile.getNumChannels();
	auto numSamples = audioFile.getNumSamplesPerChannel();
	std::vector<short> dataBuffer(numChannels * numSamples);

	for (size_t sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex) {
		for (int channel = 0; channel < numChannels; ++channel) {
			dataBuffer[sampleIndex * numChannels + channel] = audioFile.samples[channel][sampleIndex];
		}
	}

	alBufferData(soundBuffer, format, dataBuffer.data(), dataBuffer.size() * sizeof(short), audioFile.getSampleRate());
}

AudioClip::~AudioClip() {
	if (audioBuffer != 0) {
		alDeleteBuffers(1, &audioBuffer);
	}
}

ALuint AudioClip::Get() const {
	return audioBuffer;
}
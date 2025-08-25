#include <AudioFile/AudioFile.h>
#include "audio_clip.hpp"

AudioClip::AudioClip(const char* path) {
	AudioFile<short> audioFile;
	audioFile.load(path);

	alGenBuffers(1, &audioBuffer);
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

	alBufferData(audioBuffer, format, dataBuffer.data(), dataBuffer.size() * sizeof(short), audioFile.getSampleRate());
}

AudioClip::AudioClip(AudioClip&& other) noexcept : audioBuffer(other.audioBuffer) {
	other.audioBuffer = 0;
}

AudioClip::~AudioClip() {
	if (audioBuffer) {
		alDeleteBuffers(1, &audioBuffer);
	}
}

AudioClip& AudioClip::operator = (AudioClip&& other) noexcept {
	audioBuffer = other.audioBuffer;
	other.audioBuffer = 0;
	return *this;
}

ALuint AudioClip::Get() const {
	return audioBuffer;
}
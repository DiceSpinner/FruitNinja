#include <iostream>
#include <cstdlib>
#include "audio.hpp"
#include <AL/al.h>
#include <AL/alc.h>
#include <AudioFile.h>
using namespace std;

static ALCdevice* device;
static ALCcontext* context;

void initALContext() {

#ifdef _DEBUG
#if defined(_WIN32) || defined(_WIN64)
	// For Windows, use _putenv_s or _putenv
	_putenv("ALSOFT_LOG=1");
	_putenv("ALSOFT_LOG_LEVEL=3");  // Set to highest verbosity
#else
	// For Linux/macOS, use setenv
	setenv("ALSOFT_LOG", "1", 1);  // Enable logging
	setenv("ALSOFT_LOG_LEVEL", "3", 1);  // Set verbosity level (3 = debug level)
#endif
#endif

	device = alcOpenDevice("OpenAL Soft on Headphones (High Definition Audio Device)");
	const ALCchar* devices;
	const ALCchar* defaultDeviceName;
	bool ext = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT") == AL_TRUE;
	// Pass in NULL device handle to get list of devices
	devices = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
	// devices contains the device names, separated by NULL
	// and terminated by two consecutive NULLs.
	defaultDeviceName = alcGetString(NULL,
		ALC_DEFAULT_DEVICE_SPECIFIER);
	// defaultDeviceName contains the name of the default
	std::cout << "Audio Devices: " << devices << std::endl;
	if (!device)
	{
		cout << "Failed to open audio device!\n";
		exit(1);
	}
	context = alcCreateContext(device, nullptr);
	if (!context) {
		cout << "Failed to create OpenAL context!\n";
		exit(1);
	}
	if (!alcMakeContextCurrent(context)) {
		cout << "Failed to set current OpenAL context!\n";
		exit(1);
	}

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
	else{
		if (audioFile.getBitDepth() == 8) {
			format = AL_FORMAT_STEREO8;
		}
		else {
			format = AL_FORMAT_STEREO16;
		}
	}
	int numChannels = audioFile.getNumChannels();
	int numSamples = audioFile.samples.size();
	std::vector<short> dataBuffer(numChannels * numSamples);

	for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex) {
		for (int channel = 0; channel < numChannels; ++channel) {
			dataBuffer[sampleIndex * numChannels + channel] = audioFile.samples[channel][sampleIndex];
		}
	}

	alBufferData(soundBuffer, format, dataBuffer.data(), dataBuffer.size() * sizeof(short), audioFile.getSampleRate());
	
	ALuint source;
	alGenSources(1, &source);
	alSourcei(source, AL_BUFFER, soundBuffer);
	alSourcePlay(source);
	cout << alGetError();
}

void destroyALContext() {
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(context);
	alcCloseDevice(device);
}
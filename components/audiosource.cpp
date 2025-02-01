#include "audiosource.hpp"

using namespace std;

AudioSource::AudioSource(std::unordered_map<std::type_index, std::unique_ptr<Component>>& components, Transform& transform, Object* object)
	: Component(components, transform, object), audioClip(), sourceID(0)
{
	alGenSources(1, &sourceID);
}

void AudioSource::FixedUpdate() {
	auto pos = transform.position();
	alSource3f(sourceID, AL_POSITION, pos.x, pos.y, pos.z);
}

void AudioSource::SetAudioClip(std::shared_ptr<AudioClip> clip) {
	audioClip = clip;
	ALuint buffer = audioClip->Get();
	alSourcei(sourceID, AL_BUFFER, buffer);
}

void AudioSource::Play() const {
	alSourcePlay(sourceID);
}

void AudioSource::Pause() const {
	alSourceStop(sourceID);
}
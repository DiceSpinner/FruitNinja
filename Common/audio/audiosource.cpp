#include <list>
#include "audiosource.hpp"
#include "infrastructure/object.hpp"

using namespace std;

static list<ALuint>* sourcesToBeDeleted = new list<ALuint>;

AudioSource::AudioSource(unordered_map<type_index, vector<unique_ptr<Component>>>& components, Transform& transform, Object* object)
	: Component(components, transform, object), audioClip(), sourceID(0), loopEnabled(false), disableWhileNotPlaying(false)
{
	alGenSources(1, &sourceID);
}

void AudioSource::FixedUpdate(Clock& clock) {
	auto pos = transform.position();
	alSource3f(sourceID, AL_POSITION, pos.x, pos.y, pos.z);
}

void AudioSource::Update(Clock& clock) {
	if (disableWhileNotPlaying) {
		ALint result;
		alGetSourcei(sourceID, AL_SOURCE_STATE, &result);
		if (result != AL_PLAYING) {
			object->Detach();
		}
	}
}

void AudioSource::SetAudioClip(std::shared_ptr<AudioClip>& clip) {
	audioClip = clip;
	ALuint buffer = audioClip->Get();
	alSourcei(sourceID, AL_BUFFER, buffer);
}

void AudioSource::OnDisabled() {
	alSourceStop(sourceID);
}

AudioSource::~AudioSource() {
	alDeleteSources(1, &sourceID);
}

void AudioSource::Play() const {
	if (!object->IsActive()) return;
	alSourcePlay(sourceID);
}

void AudioSource::Pause() const {
	alSourceStop(sourceID);
}

void AudioSource::SetLoopEnabled(bool value) {
	loopEnabled = value;
	alSourcei(sourceID, AL_LOOPING, value);
}

bool AudioSource::LoopEnabled() const {
	return loopEnabled;
}
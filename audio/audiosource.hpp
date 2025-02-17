#ifndef AUDIOSOURCE_H
#define AUDIOSOURCE_H
#include <memory>
#include "../core/component.hpp"
#include "audio_clip.hpp"

class AudioSource : public Component {
private:
	std::shared_ptr<AudioClip> audioClip;
	ALuint sourceID;
	bool loopEnabled;
public:
	bool disableWhileNotPlaying;

	AudioSource(std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& components, Transform& transform, Object* object);
	void FixedUpdate() override;
	void Update() override;
	void SetAudioClip(std::shared_ptr<AudioClip>& clip);
	void SetLoopEnabled(bool value);
	void Play() const;
	void Pause() const;
	bool LoopEnabled() const;
	void OnDisabled() override;
	~AudioSource() override;
};
#endif
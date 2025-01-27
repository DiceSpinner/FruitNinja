#ifndef AUDIOSOURCE_H
#define AUDIOSOURCE_H
#include <memory>
#include "../core/component.hpp"
#include "../audio/audio.hpp"

class AudioSource : public Component {
private:
	std::shared_ptr<AudioClip> audioClip;
	ALuint sourceID;
public:
	AudioSource(std::unordered_map<std::type_index, std::unique_ptr<Component>>& collection, Transform& transform, Object* object);
	void FixedUpdate() override;
	void SetAudioClip(std::shared_ptr<AudioClip> clip);
	void Play() const;
	void Pause() const;
};
#endif
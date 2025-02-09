#ifndef FRUIT_H
#define FRUIT_H
#include <memory>
#include "../core/component.hpp"
#include "../rendering/model.hpp"
#include "../audio/audio_clip.hpp"

class Fruit : public Component {
private:
	bool CursorInContact();
	std::shared_ptr<Model> slice1;
	std::shared_ptr<Model> slice2;
	std::shared_ptr<AudioClip> clipOnSliced;
	std::shared_ptr<AudioClip> clipOnMissed;
public:
	int reward;
	float radius;

	Fruit(std::unordered_map<std::type_index, std::unique_ptr<Component>>& components, Transform& transform, Object* object,
		float radius, int score, 
		std::shared_ptr<Model> slice1, 
		std::shared_ptr<Model> slice2,
		std::shared_ptr<AudioClip> clipOnSliced = {},
		std::shared_ptr<AudioClip> clipOnMissed = {});
	void Update() override;
};

#endif
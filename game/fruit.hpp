#ifndef FRUIT_H
#define FRUIT_H
#include <memory>
#include "../core/component.hpp"
#include "../rendering/model.hpp"
#include "../audio/audio_clip.hpp"
#include "../core/object_pool.hpp"

class Fruit : public Component {
private:
	bool CursorInContact();
	std::shared_ptr<Model> slice1;
	std::shared_ptr<Model> slice2;
	std::shared_ptr<AudioClip> clipOnSliced;
	std::shared_ptr<AudioClip> clipOnMissed;
	void PlayVFX() const;
public:
	int reward;
	float radius;
	glm::vec4 color;
	ObjectPool<Object>& particlePool;
	GLuint slicedParticleTexture;

	Fruit(std::unordered_map<std::type_index, std::unique_ptr<Component>>& components, Transform& transform, Object* object,
		float radius, int score,
		ObjectPool<Object>& particlePool,
		std::shared_ptr<Model> slice1, 
		std::shared_ptr<Model> slice2,
		std::shared_ptr<AudioClip> clipOnSliced = {},
		std::shared_ptr<AudioClip> clipOnMissed = {});
	void Update() override;
};

#endif
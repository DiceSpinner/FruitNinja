#ifndef FRUIT_H
#define FRUIT_H
#include <memory>
#include <functional>
#include "infrastructure/object.hpp"
#include "rendering/model.hpp"
#include "audio/audio_clip.hpp"
#include "infrastructure/object_pool.hpp"

struct FruitChannel {
	int score = 0;
	int miss = 0;
	int recovery = 0;
	bool recentlyRecovered = false;
	bool enableSlicing = true;
	bool disableNonUI = false;
	float killHeight = 0;
	std::shared_ptr<ObjectPool<Object>> particleSystemPool;
};

struct FruitAsset {
	std::shared_ptr<Model> slice1;
	std::shared_ptr<Model> slice2;
	std::shared_ptr<AudioClip> clipOnSliced;
	std::shared_ptr<AudioClip> clipOnMissed;
};

class Fruit : public Component {
private:
	void PlayVFX() const;
	FruitChannel& channel;
	FruitAsset assets;
public:
	int reward;
	float radius;
	float sliceForce;
	glm::vec4 color;
	GLuint slicedParticleTexture;

	Fruit(
		std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& components, Transform& transform, Object* object,
		float radius, int score, float sliceForce, FruitChannel& channel, const FruitAsset& assets
	);
	void Update(const Clock& clock) override;
};

#endif
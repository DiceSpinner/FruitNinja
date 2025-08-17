#ifndef SLICABLE_H
#define SLICABLE_H
#include <functional>
#include "audio/audio_clip.hpp"
#include "infrastructure/object.hpp"
#include "infrastructure/object_pool.hpp"

struct SlicableAsset {
	std::shared_ptr<Model> topSlice;
	std::shared_ptr<Model> bottomSlice;
	std::shared_ptr<AudioClip> clipOnSliced;
	std::shared_ptr<AudioClip> clipOnMissed;
};

struct SlicableControl {
	bool disableSlicing = false;
	bool disableAll = false;
	float killHeight = 0;

	std::shared_ptr<ObjectPool<Object>> particlePool;
};

class Sliced : public Component {
private:
	std::shared_ptr<SlicableControl> control;
public:
	Sliced(std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& components, Transform& transform, Object* object, 
		const std::shared_ptr<SlicableControl>& control);
	void Update(const Clock& clock) override;
};

class Slicable : public Component {
	SlicableAsset asset;
	std::shared_ptr<SlicableControl> control;
	
	float radius;
	float sliceForce;
	bool sliced = false;

	void PlayParticleVFX() const;
public:
	bool destroyOnSliced = true;
	GLuint particleTexture = 0;
	glm::vec4 particleColor = { 1, 1, 1, 1 };
	std::function<void()> onMissed;
	std::function<void(Transform& transform, glm::vec3 up)> onSliced;

	Slicable(
		std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& components, Transform& transform, Object* object,
		float radius, float sliceForce, const std::shared_ptr<SlicableControl>& control, const SlicableAsset& assets
	);
	void OnEnabled() override;

	void Update(const Clock& clock) override;

	void Slice(const Clock& clock, glm::vec3 up);
};

#endif
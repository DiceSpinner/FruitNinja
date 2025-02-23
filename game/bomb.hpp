#ifndef BOMB_H
#define BOMB_H
#include <AL/al.h>
#include "../audio/audiosource_pool.hpp"
#include "../core/component.hpp"
#include "../rendering/particle_system.hpp"

class Bomb : public Component {
private:
	std::shared_ptr<AudioClip> explosionSFX;
	float radius;
public:
	Bomb(std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& collection, Transform& transform, Object* object,
		std::shared_ptr<AudioClip> explosionSFX, float radius);
	void Update() override;
};
#endif
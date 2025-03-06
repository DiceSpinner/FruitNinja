#ifndef BOMB_H
#define BOMB_H
#include <AL/al.h>
#include "../audio/audiosource_pool.hpp"
#include "../core/component.hpp"
#include "../rendering/particle_system.hpp"

struct BombChannel {
	float killHeight = 0;
	bool enableSlicing = true;
	bool disableAll = false;
	bool bombHit = false;
	glm::vec3 explosionPosition = {};
};

class Bomb : public Component {
private:
	std::shared_ptr<AudioClip> explosionSFX;
	BombChannel& channel;
	float radius;
public:
	Bomb(std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& collection, Transform& transform, Object* object,
		std::shared_ptr<AudioClip> explosionSFX, float radius, BombChannel& channel);
	void Update() override;
};
#endif
#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H
#include <glad/glad.h>
#include <list>
#include <functional>
#include "mesh.hpp"
#include "../core/component.hpp"

struct Particle {
	glm::vec3 pos;
	glm::vec3 velocity;
	glm::vec3 scale;
	glm::vec4 color;
	float timeLived;
	float lifeTime;
};

class ParticleSystem : public Component {
private:
	GLuint VAO, quadBuffer, offsetVertexBuffer, localScaleBuffer, colorModifierBuffer;
	std::list<Particle> inactiveParticles;
	std::list<Particle> activeParticles;
	std::function<void(Particle&)> particleModifier;
	unsigned int maxCount;
	float maxLifeTime;
	float minLifeTime;
	float spawnCounter;
	bool init;
public:
	GLuint texture;
	bool useGravity, is3D, disableOnFinish;
	float spawnFrequency;
	unsigned int spawnAmount;
	glm::vec3 spawnDirection;
	float maxSpawnDirectionDeviation;
	glm::vec3 scale;
	glm::vec4 color;

	static void DrawParticles(Shader& shader);

	ParticleSystem(
		std::unordered_map<std::type_index, std::unique_ptr<Component>>& collection, Transform& transform, Object* object, 
		unsigned int maxParticleCount, std::function<void(Particle&)> particleModifier = {}
	);
	void SpawnParticle(glm::vec3 pos, glm::vec3 velocity);
	void SetParticleLifeTime(float min, float max);
	void Draw(Shader& shader) const;
	void FixedUpdate() override;
	void OnEnabled() override;
	void OnDisabled() override;
	~ParticleSystem() override;
};

#endif
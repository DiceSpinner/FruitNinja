#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H
#include <glad/glad.h>
#include <list>
#include <functional>
#include "mesh.hpp"
#include "infrastructure/object.hpp"
#include "renderer.hpp"

struct Particle {
	glm::vec3 pos;
	glm::vec3 velocity;
	glm::vec2 up;
	glm::vec3 scale;
	glm::vec4 color;
	float timeLived;
	float lifeTime;
};

class ParticleSystem : public Component {
private:
	GLuint VAO, quadBuffer, positionVertexBuffer, localScaleBuffer, colorModifierBuffer, upDirectionBuffer;
	std::list<Particle> inactiveParticles;
	std::list<Particle> activeParticles;
	std::function<void(Particle&, ParticleSystem&)> particleModifier;
	unsigned int maxCount;
	float maxLifeTime;
	float minLifeTime;
	float spawnCounter;
	bool init;
public:
	glm::vec3 relativeOffset;
	glm::vec3 absoluteOffset;
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
		std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& collection, Transform& transform, Object* object,
		unsigned int maxParticleCount, std::function<void(Particle&, ParticleSystem&)> particleModifier = {}
	);
	void SpawnParticle();
	void SetParticleLifeTime(float min, float max);
	void Draw(Shader& shader) const;
	void FixedUpdate(const Clock& clock) override;
	void OnEnabled() override;
	void OnDisabled() override;
	~ParticleSystem() override;
};

#endif
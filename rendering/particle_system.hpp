#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H
#include <glad/glad.h>
#include <list>
#include "mesh.hpp"
#include "../core/component.hpp"

class ParticleSystem : public Component {
private:
	struct Particle {
		glm::vec3 pos;
		glm::vec3 velocity;
		glm::vec3 scale;
		float timeLived;
	};
	GLuint VAO, quadBuffer, offsetVertexBuffer;

	std::list<Particle> inactiveParticles;
	std::list<Particle> activeParticles;
	unsigned int maxCount;
	float maxLifeTime;
	GLuint texture;
public:
	bool useGravity;

	static void DrawParticles(Shader& shader);

	ParticleSystem(std::unordered_map<std::type_index, std::unique_ptr<Component>>& collection, Transform& transform, Object* object, unsigned int maxParticleCount, float maxLifeTime);
	void SetTexture(GLuint texture);
	void Draw(Shader& shader) const;
	void FixedUpdate() override;
	void OnEnabled() override;
	void OnDisabled() override;
	~ParticleSystem() override;
};

#endif
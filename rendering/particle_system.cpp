#include <glm/ext.hpp>
#include "../state/time.hpp"
#include "particle_system.hpp"

using namespace std;

static const GLfloat quadData[] = {
 -0.5f, -0.5f, 0.0f,
 0.5f, -0.5f, 0.0f,
 -0.5f, 0.5f, 0.0f,
 0.5f, 0.5f, 0.0f,
};

static vector<ParticleSystem*>* activeSystems = new vector<ParticleSystem*>;
static glm::vec3 gravity(0, -20, 0);

void ParticleSystem::DrawParticles(Shader& shader) {
	for (auto system : *activeSystems) {
		system->Draw(shader);
	}
}

ParticleSystem::ParticleSystem(unordered_map<type_index, unique_ptr<Component>>& collection, Transform& transform, Object* object, unsigned int maxParticleCount, float maxLifeTime)
	: Component(collection, transform, object), maxCount(maxParticleCount), maxLifeTime(maxLifeTime), 
	inactiveParticles(maxParticleCount), activeParticles(0), texture(0), useGravity(true),
	VAO(0), offsetVertexBuffer(0), quadBuffer(0)
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glEnableVertexAttribArray(0);
	glGenBuffers(1, &quadBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quadBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glEnableVertexAttribArray(1);
	glGenBuffers(1, &offsetVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, offsetVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * maxCount, nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	
	glVertexAttribDivisor(0, 0);
	glVertexAttribDivisor(1, 1);
	
	glBindVertexArray(0);
}

void ParticleSystem::SetTexture(GLuint texture) {
	this->texture = texture;
}

void ParticleSystem::FixedUpdate() {
	if (!inactiveParticles.empty()) {
		activeParticles.splice(activeParticles.end(), inactiveParticles, inactiveParticles.begin());
		auto& particle = activeParticles.back();
		particle.timeLived = 0;
		particle.pos = glm::vec3(0, 0, 0);
		particle.velocity = glm::vec3(glm::sin(time() * 10) * 5, 5, 0);
	}

	float dt = fixedDeltaTime();
	glBindBuffer(GL_ARRAY_BUFFER, offsetVertexBuffer);
	glm::vec3* buffer = static_cast<glm::vec3*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
	unsigned int index = 0;
	for (auto i = activeParticles.begin(); i != activeParticles.end();) {
		i->timeLived += dt;
		if (i->timeLived >= maxLifeTime) {
			auto nextItem = next(i);
			inactiveParticles.splice(inactiveParticles.end(), activeParticles, i);
			i = nextItem;
		}
		else {
			if (useGravity) {
				i->velocity += dt * gravity;
			}
			i->pos += i->velocity * dt;
			buffer[index++] = i->pos;
			i++;
		}
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);
}

void ParticleSystem::Draw(Shader& shader) const {
	if (!texture) {
		return;
	}
	glActiveTexture(GL_TEXTURE0);
	shader.SetInt("image", 0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform3fv(glGetUniformLocation(shader.ID, "modelPos"), 1, glm::value_ptr(transform.position()));

	glBindVertexArray(VAO);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, activeParticles.size());
	glBindVertexArray(0);
}

void ParticleSystem::OnEnabled() {
	activeSystems->push_back(this);
}

void ParticleSystem::OnDisabled() {
	erase(*activeSystems, this);
}

ParticleSystem::~ParticleSystem() {
	glDeleteBuffers(1, &quadBuffer);
	glDeleteBuffers(1, &offsetVertexBuffer);
	glDeleteVertexArrays(1, &VAO);
}
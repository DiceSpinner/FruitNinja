#include <glm/ext.hpp>
#include "../state/time.hpp"
#include "particle_system.hpp"
#include "renderer.hpp"
#include "../core/object.hpp"

using namespace std;
using namespace Time;

static const GLfloat quadData[] = {
 -0.5f, -0.5f, 0.0f,
 0.5f, -0.5f, 0.0f,
 -0.5f, 0.5f, 0.0f,
 0.5f, 0.5f, 0.0f,
};

static vector<ParticleSystem*>* activeSystems = new vector<ParticleSystem*>;
static glm::vec3 Gravity(0, -15, 0);

static float randFloat(float min, float max) {
	return rand() / static_cast<float>(RAND_MAX) * (max - min) + min;
}

static glm::vec3 randomUnitVectorInCone(const glm::vec3& direction, float maxAngle) {
	if (direction == glm::vec3(0, 0, 0) || maxAngle == 0) {
		return direction;
	}
	glm::vec3 result = glm::sphericalRand(1.0f);
	auto allowance = glm::radians(maxAngle);
	while (glm::acos(glm::dot(result, direction)) > allowance) {
		result = glm::sphericalRand(1.0f);
	};
	return result;
}

static glm::vec3 randomUnitVectorInCircle(const glm::vec3& direction, float maxAngle) {
	if (direction == glm::vec3(0, 0, 0) || maxAngle == 0) {
		return direction;
	}
	glm::vec3 result = glm::vec3(glm::circularRand(1.0f), 0);
	auto allowance = glm::radians(maxAngle);
	while (glm::acos(glm::dot(result, direction)) > allowance) {
		result = glm::vec3(glm::circularRand(1.0f), 0);
	};
	return result;
}

void ParticleSystem::DrawParticles(Shader& shader) {
	for (auto system : *activeSystems) {
		system->Draw(shader);
	}
}

ParticleSystem::ParticleSystem(unordered_map<type_index, vector<unique_ptr<Component>>>& collection, Transform& transform, Object* object, unsigned int maxParticleCount, function<void(Particle&, ParticleSystem&)> particleModifier)
	: Component(collection, transform, object), maxCount(maxParticleCount), minLifeTime(1), maxLifeTime(1), particleModifier(particleModifier), init(true),
	inactiveParticles(maxParticleCount), activeParticles(0), texture(0), useGravity(true), is3D(true), disableOnFinish(false),
	relativeOffset(0), maxSpawnDirectionDeviation(45), spawnDirection(0, 1, 0), color(1, 1, 1, 1),
	spawnFrequency(1), spawnCounter(0), scale(1, 1, 1), spawnAmount(0), absoluteOffset(0),
	VAO(0), positionVertexBuffer(0), quadBuffer(0), localScaleBuffer(0), upDirectionBuffer(0)
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glEnableVertexAttribArray(0);
	glGenBuffers(1, &quadBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quadBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glEnableVertexAttribArray(1);
	glGenBuffers(1, &positionVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, positionVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * maxCount, nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glEnableVertexAttribArray(2);
	glGenBuffers(1, &localScaleBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, localScaleBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * maxCount, nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glEnableVertexAttribArray(3);
	glGenBuffers(1, &colorModifierBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorModifierBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * maxCount, nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glEnableVertexAttribArray(4);
	glGenBuffers(1, &upDirectionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, upDirectionBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * maxCount, nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	
	glVertexAttribDivisor(0, 0);
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);

	glBindVertexArray(0);
}

void ParticleSystem::SetParticleLifeTime(float min, float max) {
	if (min <= max) {
		minLifeTime = min;
		maxLifeTime = max;
	}
}

void ParticleSystem::SpawnParticle() {
	if (!inactiveParticles.empty()) {
		activeParticles.splice(activeParticles.end(), inactiveParticles, inactiveParticles.begin());
		auto& particle = activeParticles.back();
		particle.timeLived = 0;
		particle.lifeTime = randFloat(minLifeTime, maxLifeTime);
		particle.pos = transform.position() + absoluteOffset + glm::vec3(transform.rotation() * glm::vec4(relativeOffset, 0));
		particle.velocity = glm::length(spawnDirection) * randomUnitVectorInCone(transform.rotation() * glm::vec4(spawnDirection, 0), maxSpawnDirectionDeviation);
		particle.color = glm::vec4(1, 1, 1, 0);
		particle.up = particle.velocity;

		if (particleModifier) {
			particleModifier(particle, *this);
		}
	}
}

void ParticleSystem::FixedUpdate() {
	if (init) {
		init = false;
		for (auto i = 0; i < spawnAmount; i++) {
			SpawnParticle();
		}
		return;
	}
	if (disableOnFinish && !spawnFrequency && activeParticles.empty()) {
		object->SetEnable(false);
		return;
	}

	float dt = fixedDeltaTime();
	spawnCounter = spawnFrequency ? spawnCounter + dt : 0;

	auto spawnFreqRecp = spawnFrequency ? (1 / spawnFrequency) : 1;
	while(spawnCounter >= spawnFreqRecp) {
		for (auto i = 0; i < spawnAmount; i++) {
			SpawnParticle();
		}
		spawnCounter -= spawnFreqRecp;
	}

	glBindBuffer(GL_ARRAY_BUFFER, positionVertexBuffer);
	glm::vec3* position = static_cast<glm::vec3*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
	glBindBuffer(GL_ARRAY_BUFFER, localScaleBuffer);
	glm::vec3* localScale = static_cast<glm::vec3*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
	glBindBuffer(GL_ARRAY_BUFFER, colorModifierBuffer);
	glm::vec4* colorBuffer = static_cast<glm::vec4*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
	glBindBuffer(GL_ARRAY_BUFFER, upDirectionBuffer);
	glm::vec2* upBuffer = static_cast<glm::vec2*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
	
	// Update position and physics
	unsigned int index = 0;
	for (auto i = activeParticles.begin(); i != activeParticles.end();) {
		i->timeLived += dt;
		if (i->timeLived >= i->lifeTime) {
			auto nextItem = next(i);
			inactiveParticles.splice(inactiveParticles.end(), activeParticles, i);
			i = nextItem;
		}
		else {
			if (useGravity) {
				i->velocity += dt * Gravity;
			}
			i->pos += i->velocity * dt;

			if (!particleModifier) {	
				float quater = 0.2 * i->lifeTime;
				float remainder = 0.8 * i->lifeTime;
				float opacity = i->timeLived < quater ? i->timeLived / quater : (remainder - i->timeLived + quater) / remainder;
				float size = i->timeLived < quater ? i->timeLived / quater : 1;
				i->scale = glm::vec3(size, size, size);
				i->color = color * glm::vec4(1, 1, 1, opacity);
			}
			else {
				particleModifier(*i, *this);
			}

			position[index] = i->pos;
			colorBuffer[index] = i->color;
			upBuffer[index] = i->up;
			localScale[index++] = i->scale;
			i++;
		}
	}

	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, colorModifierBuffer);
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, localScaleBuffer);
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, positionVertexBuffer);
	glUnmapBuffer(GL_ARRAY_BUFFER);
}

void ParticleSystem::Draw(Shader& shader) const {
	if (texture) {
		glActiveTexture(GL_TEXTURE0);
		shader.SetInt("image", 0);
		glBindTexture(GL_TEXTURE_2D, texture);
	}
	else {
		shader.SetInt("image", Renderer::white);
	}
	
	glUniform3fv(glGetUniformLocation(shader.ID, "scale"), 1, glm::value_ptr(scale));

	glBindVertexArray(VAO);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, activeParticles.size());
	glBindVertexArray(0);
}

void ParticleSystem::OnEnabled() {
	activeSystems->push_back(this);
	init = true;
}

void ParticleSystem::OnDisabled() {
	erase(*activeSystems, this);
}

ParticleSystem::~ParticleSystem() {
	glDeleteBuffers(1, &quadBuffer);
	glDeleteBuffers(1, &positionVertexBuffer);
	glDeleteVertexArrays(1, &VAO);
}
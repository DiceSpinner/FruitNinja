#include <glad/glad.h>
#include <vector>
#include "vfx.hpp"
#include "../core/transform.hpp"

using namespace std;

static glm::vec3 randomUnitVectorNotInDoubleCone(const glm::vec3& direction, float maxAngle) {
	if (direction == glm::vec3(0, 0, 0) || maxAngle == 0) {
		return direction;
	}
	glm::vec3 result = glm::sphericalRand(1.0f);
	auto allowance = glm::radians(maxAngle);
	while (glm::acos(glm::dot(result, direction)) <= allowance || glm::acos(glm::dot(result, -direction)) <= allowance) {
		result = glm::sphericalRand(1.0f);
	};
	return glm::normalize(result);
}

static glm::vec3 randomUnitVectorInDoubleCone(const glm::vec3& direction, float maxAngle) {
	if (direction == glm::vec3(0, 0, 0) || maxAngle == 0) {
		return direction;
	}
	glm::vec3 result = glm::sphericalRand(1.0f);
	auto allowance = glm::radians(maxAngle);
	while (glm::acos(glm::dot(result, direction)) > allowance && glm::acos(glm::dot(result, -direction)) > allowance) {
		result = glm::sphericalRand(1.0f);
	};
	return glm::normalize(result);
}

ExplosionVFX::ExplosionVFX() {
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glEnableVertexAttribArray(0);
	glGenBuffers(1, &posBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
	glBufferData(GL_ARRAY_BUFFER, numRays * 3 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glEnableVertexAttribArray(1);
	glGenBuffers(1, &colorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
	glBufferData(GL_ARRAY_BUFFER, numRays * 3 * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glBindVertexArray(0);

	for (auto i = 0; i < numRays; i++){
		glm::vec3 rayDir = randomUnitVectorNotInDoubleCone(glm::vec3(0, 0, 1), 50);
	setRayDir:
		for (auto j = 0; j < rayDirections.size();j ++) {
			if (glm::acos(glm::dot(rayDirections[j], rayDir)) < glm::radians(30.0f)) {
				rayDir = randomUnitVectorNotInDoubleCone(glm::vec3(0, 0, 1), 40);
				goto setRayDir;
			}
		}

		glm::mat4 rot(1.0f);
		glm::vec3 rotDir = randomUnitVectorInDoubleCone(glm::vec3(0, 0, 1), 30);
		rot = glm::rotate(rot, glm::radians(raySectorWidth), rotDir);
		
		rayDirections.push_back(rot * glm::vec4(rayDir, 0));
		rayDirections.push_back(glm::transpose(rot) * glm::vec4(rayDir, 0));
	}
}

ExplosionVFX::~ExplosionVFX() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &posBuffer);
	glDeleteBuffers(1, &colorBuffer);
}

void ExplosionVFX::Draw(Shader& shader) {
	glBindVertexArray(VAO);
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "transform"), 1, GL_FALSE, glm::value_ptr(transform.matrix));

	glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
	glm::vec3* posBuff = static_cast<glm::vec3*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
	glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
	glm::vec4* colorBuff = static_cast<glm::vec4*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

	for (auto i = 0; i < numRays;i++) {
		posBuff[3 * i] = glm::vec3(0, 0, 0);
		posBuff[3 * i + 1] = rayLength* rayDirections[2 * i];
		posBuff[3 * i + 2] = rayLength * rayDirections[2 * i + 1];

		colorBuff[3 * i] = rayColor;
		colorBuff[3 * i + 1] = rayOpacity * rayColor;
		colorBuff[3 * i + 2] = rayOpacity * rayColor;
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
	glUnmapBuffer(GL_ARRAY_BUFFER);

	glDrawArrays(GL_TRIANGLES, 0, numRays * 3);
	glBindVertexArray(0);
}
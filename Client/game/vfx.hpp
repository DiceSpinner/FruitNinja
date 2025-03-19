#ifndef VFX_H
#define VFX_H
#include <glm/ext.hpp>
#include <vector>
#include "infrastructure/transform.hpp"
#include "rendering/shader.hpp"
class ExplosionVFX {
private:
	size_t numRays = 8;
	GLuint VAO, posBuffer, colorBuffer;
	std::vector<glm::vec3> rayDirections;
	float raySectorWidth = 8;
public:
	Transform transform;
	float rayLength = 0;
	float rayOpacity = 0;
	glm::vec4 rayColor = { 1, 1, 1, 1 };

	ExplosionVFX();
	ExplosionVFX(const ExplosionVFX& other) = delete;
	ExplosionVFX(ExplosionVFX&& other) = delete;
	ExplosionVFX& operator = (const ExplosionVFX& other) = delete;
	ExplosionVFX& operator = (ExplosionVFX&& other) = delete;

	~ExplosionVFX();
	void Draw(Shader& shader);
};
#endif
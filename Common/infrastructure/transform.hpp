#ifndef TRANSFORM_H
#define TRANSFORM_H
#include "glm/glm.hpp"

class Transform{
public:
	glm::mat4 matrix;

	Transform();
	glm::vec3 position() const;
	glm::vec3 up() const;
	glm::vec3 right() const;
	glm::vec3 forward() const;
	glm::mat4 rotation() const;
	glm::vec3 scale() const;

	void LookAt(const glm::vec3& position, const glm::vec3& up = glm::vec3(0, 1, 0));
	void SetPosition(const glm::vec3& position);
	void SetRotation(const glm::mat4& newRotation);
	void SetScale(const glm::vec3& scale);
	void SetForward(const glm::vec3& direction);
	void SetRight(const glm::vec3& direction);
	void SetUp(const glm::vec3& direction);
};

#endif
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

	void LookAt(glm::vec3 position, glm::vec3 up = glm::vec3(0, 1, 0));
	void SetPosition(glm::vec3 position);
	void SetRotation(glm::mat4 newRotation);
	void SetScale(glm::vec3 scale);
	void SetForward(glm::vec3 direction);
	void SetRight(glm::vec3 direction);
	void SetUp(glm::vec3 direction);
};

#endif
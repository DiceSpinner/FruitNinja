#include "transform.hpp"
#include "glm/ext.hpp"

using namespace std;

Transform::Transform() : matrix(1.0f) { }

glm::vec3 Transform::position() const {
	return glm::vec3(matrix[3]);
}

glm::vec3 Transform::forward() const {
	return glm::normalize(glm::vec3(matrix * glm::vec4(0, 0, 1, 1)));
}

glm::vec3 Transform::right() const {
	return glm::normalize(glm::vec3(matrix * glm::vec4(1, 0, 0, 1)));
}

glm::vec3 Transform::up() const {
	return glm::normalize(glm::vec3(matrix * glm::vec4(0, 1, 0, 1)));
}

glm::vec3 Transform::scale() const {
	return glm::vec3(
		glm::length(matrix[0]),
		glm::length(matrix[1]),
		glm::length(matrix[2])
	);
}

glm::mat4 Transform::rotation() const {
	glm::mat4 result(matrix);
	result[0] = glm::normalize(result[0]);
	result[1] = glm::normalize(result[1]);
	result[2] = glm::normalize(result[2]);
	result[3] = glm::vec4(0, 0, 0, 1);
	return result;
}

void Transform::SetPosition(glm::vec3 position) {
	matrix[3] = glm::vec4(position, 1);
}

void Transform::SetForward(glm::vec3 direction) {
	glm::vec3 currForward = forward();
	glm::vec3 newDirection = glm::normalize(direction);

	float dot = glm::dot(currForward, newDirection);
	if (glm::abs(1.0f - dot) < 1e-6f) { // Vector aligned
		return;
	}

	glm::vec3 axis = glm::normalize(glm::cross(currForward, newDirection));

	if (glm::abs(dot + 1.0f) < 1e-6f) {
		matrix = glm::rotate(matrix, glm::pi<float>(), axis);
		return;
	}
	float angle = glm::acos(dot);
	matrix = glm::rotate(matrix, angle, axis);
}

void Transform::SetRight(glm::vec3 direction) {
	glm::vec3 currRight = right();
	glm::vec3 newDirection = glm::normalize(direction);

	float dot = glm::dot(currRight, newDirection);
	if (glm::abs(1.0f - dot) < 1e-6f) { // Vector aligned
		return;
	}

	glm::vec3 axis = glm::normalize(glm::cross(currRight, newDirection));

	if (glm::abs(dot + 1.0f) < 1e-6f) {
		matrix = glm::rotate(matrix, glm::pi<float>(), axis);
		return;
	}
	float angle = glm::acos(dot);
	matrix = glm::rotate(matrix, angle, axis);
}

void Transform::SetUp(glm::vec3 direction) {
	glm::vec3 currUp = up();
	glm::vec3 newDirection = glm::normalize(direction);

	float dot = glm::dot(currUp, newDirection);
	if (glm::abs(1.0f - dot) < 1e-6f) { // Vector aligned
		return;
	}

	glm::vec3 axis = glm::normalize(glm::cross(currUp, newDirection));

	if (glm::abs(dot + 1.0f) < 1e-6f) {
		matrix = glm::rotate(matrix, glm::pi<float>(), axis);
		return;
	}
	float angle = glm::acos(dot);
	matrix = glm::rotate(matrix, angle, axis);
}

void Transform::SetScale(glm::vec3 scaling) {
	matrix[0] = glm::normalize(matrix[0]);
	matrix[1] = glm::normalize(matrix[1]);
	matrix[2] = glm::normalize(matrix[2]);
	matrix = glm::scale(matrix, scaling);
}

void Transform::SetRotation(glm::mat4 newRotation) {
	glm::vec3 currScale = scale();
	matrix[0] = currScale.x * glm::normalize(newRotation[0]);
	matrix[1] = currScale.y * glm::normalize(newRotation[1]);
	matrix[2] = currScale.z * glm::normalize(newRotation[2]);
}
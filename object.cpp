#include "object.hpp"
#include "glm/ext.hpp"

Object::Object(Model& model) : model(&model), transform(1.0), alive(true) {
}

void Object::draw(Shader& shader) const {
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "transform"), 1, GL_FALSE, glm::value_ptr(transform));
	model->Draw(shader);
}

glm::vec3 Object::position() const {
	return glm::vec3(transform[3]);
}

glm::vec3 Object::forward() const {
	return glm::normalize(glm::vec3(transform * glm::vec4(0, 0, 1, 1)));
}

glm::vec3 Object::right() const {
	return glm::normalize(glm::vec3(transform * glm::vec4(1, 0, 0, 1)));
}

glm::vec3 Object::up() const {
	return glm::normalize(glm::vec3(transform * glm::vec4(0, 1, 0, 1)));
}

glm::vec3 Object::scale() const {
	return glm::vec3(
		glm::length(transform[0]), 
		glm::length(transform[1]), 
		glm::length(transform[2])
	);
}

glm::mat4 Object::rotation() const {
	glm::mat4 result(transform);
	result[0] = glm::normalize(result[0]);
	result[1] = glm::normalize(result[1]);
	result[2] = glm::normalize(result[2]);
	result[3] = glm::vec4(0, 0, 0, 1);
	return result;
}

bool Object::isAlive() const { return alive; }

void Object::setPosition(glm::vec3 position) {
	transform[3] = glm::vec4(position, 1);
}

void Object::setForward(glm::vec3 direction) {
	glm::vec3 currForward = forward();
	glm::vec3 newDirection = glm::normalize(direction);

	float dot = glm::dot(currForward, newDirection);
	if (glm::abs(1.0f - dot) < 1e-6f) { // Vector aligned
		return;
	}

	glm::vec3 axis = glm::normalize(glm::cross(currForward, newDirection));
	
	if (glm::abs(dot + 1.0f) < 1e-6f) {
		transform = glm::rotate(transform, glm::pi<float>(), axis);
		return;
	}
	float angle = glm::acos(dot);
	transform = glm::rotate(transform, angle, axis);
}

void Object::setRight(glm::vec3 direction) {
	glm::vec3 currRight = right();
	glm::vec3 newDirection = glm::normalize(direction);

	float dot = glm::dot(currRight, newDirection);
	if (glm::abs(1.0f - dot) < 1e-6f) { // Vector aligned
		return;
	}

	glm::vec3 axis = glm::normalize(glm::cross(currRight, newDirection));

	if (glm::abs(dot + 1.0f) < 1e-6f) {
		transform = glm::rotate(transform, glm::pi<float>(), axis);
		return;
	}
	float angle = glm::acos(dot);
	transform = glm::rotate(transform, angle, axis);
}

void Object::setUp(glm::vec3 direction) {
	glm::vec3 currUp = up();
	glm::vec3 newDirection = glm::normalize(direction);

	float dot = glm::dot(currUp, newDirection);
	if (glm::abs(1.0f - dot) < 1e-6f) { // Vector aligned
		return;
	}

	glm::vec3 axis = glm::normalize(glm::cross(currUp, newDirection));

	if (glm::abs(dot + 1.0f) < 1e-6f) {
		transform = glm::rotate(transform, glm::pi<float>(), axis);
		return;
	}
	float angle = glm::acos(dot);
	transform = glm::rotate(transform, angle, axis);
}

void Object::setScale(glm::vec3 scaling) {
	transform[0] = glm::normalize(transform[0]);
	transform[1] = glm::normalize(transform[1]);
	transform[2] = glm::normalize(transform[2]);
	transform = glm::scale(transform, scaling);
}

void Object::setRotation(glm::mat4 newRotation) {
	glm::vec3 currScale = scale();
	transform[0] = currScale.x * glm::normalize(newRotation[0]);
	transform[1] = currScale.y * glm::normalize(newRotation[1]);
	transform[2] = currScale.z * glm::normalize(newRotation[2]);
}

void Object::update() {}

void Object::destroy() { alive = false; }
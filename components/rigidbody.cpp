#include <iostream>
#include "rigidbody.hpp"
#include "../state/time.hpp"
#include "glm/ext.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace std;

const glm::vec3 Rigidbody::Gravity(0, -2, 0);
Rigidbody::Rigidbody(std::unordered_map<std::type_index, std::unique_ptr<Component>>& collection, Transform& transform) :
	Component(collection, transform), velocity(0) {}

void Rigidbody::AddForce(glm::vec3 force, ForceMode forcemode) {
	if (forcemode == ForceMode::Force) {
		velocity += deltaTime() * force;
	}
	else {
		velocity += force;
	}
}

void Rigidbody::Update() {
	transform.matrix = glm::translate(transform.matrix, velocity * deltaTime());
	velocity += (Gravity * deltaTime());
}
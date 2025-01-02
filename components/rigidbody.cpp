#include <iostream>
#include "rigidbody.hpp"
#include "../state/time.hpp"
#include "glm/ext.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace std;

const glm::vec3 Rigidbody::Gravity(0, -4, 0);
Rigidbody::Rigidbody(unordered_map<type_index, unique_ptr<Component>>& collection, 
	Transform& transform, Object* object) :
	Component(collection, transform, object), velocity(0) {}

void Rigidbody::AddForce(glm::vec3 force, ForceMode forcemode) {
	if (forcemode == ForceMode::Force) {
		velocity += deltaTime() * force;
	}
	else {
		velocity += force;
	}
}

void Rigidbody::Update() {
	glm::vec3 translation = transform.position();
	translation += velocity * deltaTime();
	transform.matrix[3] = glm::vec4(translation, 1);
	velocity += (Gravity * deltaTime());
}
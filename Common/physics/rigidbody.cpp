#include <iostream>
#include "rigidbody.hpp"
#include "glm/ext.hpp"
#include <glm/gtc/epsilon.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace std;

const glm::vec3 Rigidbody::Gravity(0, -25, 0);
Rigidbody::Rigidbody(unordered_map<type_index, vector<unique_ptr<Component>>>& components, Transform& transform, Object* object) :
	Component(components, transform, object), velocity(0), localAngularVelocity(0), useGravity(true) {}

void Rigidbody::AddForce(const Clock& clock, glm::vec3 force, ForceMode forcemode) {
	if (forcemode == ForceMode::Force) {
		velocity += clock.FixedDeltaTime() * force;
	}
	else if(forcemode == ForceMode::Impulse) {
		velocity += force;
	}
}

void Rigidbody::AddTorque(const Clock& clock, glm::vec3 torque, ForceMode forcemode) {
	glm::vec4 force(torque, 0);
	force = glm::inverse(transform.matrix) * force;
	glm::vec3 localTorque(force);
	if (forcemode == ForceMode::Force) {
		localAngularVelocity += clock.FixedDeltaTime() * localTorque;
	}
	else {
		localAngularVelocity += localTorque;
	}
}

void Rigidbody::AddRelativeTorque(const Clock& clock, glm::vec3 torque, ForceMode forcemode) {
	if (forcemode == ForceMode::Force) {
		localAngularVelocity += clock.FixedDeltaTime() * torque;
	}
	else {
		localAngularVelocity += torque;
	}
}

void Rigidbody::EarlyFixedUpdate(const Clock& clock) {
	glm::vec3 translation = transform.position();
	translation += velocity * clock.FixedDeltaTime();
	transform.matrix[3] = glm::vec4(translation, 1);
	float rotation = glm::length(localAngularVelocity);
	
	if (!glm::all(glm::epsilonEqual(localAngularVelocity, glm::vec3(0.0f), (float)1e-6))) {
		transform.matrix = glm::rotate(transform.matrix, glm::radians(rotation * clock.FixedDeltaTime()), localAngularVelocity);
	}
	if (useGravity) {
		velocity += (Gravity * clock.FixedDeltaTime());
	}
}
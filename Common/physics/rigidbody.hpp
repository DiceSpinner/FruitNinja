#ifndef RIGIDBODY_H
#define RIGIDBODY_H
#include "infrastructure/object.hpp"

enum ForceMode {
	Force,
	Impulse
};

class Rigidbody : public Component {
	static const glm::vec3 Gravity;
public:
	Rigidbody(std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& components, Transform& transform, Object* object);
	glm::vec3 velocity;
	glm::vec3 localAngularVelocity;
	bool useGravity;

	void AddForce(Clock& clock, glm::vec3 force, ForceMode forcemode = ForceMode::Force);
	void AddTorque(Clock& clock, glm::vec3 torque, ForceMode forcemode = ForceMode::Force);
	void AddRelativeTorque(Clock& clock, glm::vec3 torque, ForceMode forcemode = ForceMode::Force);
	void EarlyFixedUpdate(Clock& clock) override;
};

#endif
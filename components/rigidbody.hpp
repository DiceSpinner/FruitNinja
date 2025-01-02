#ifndef RIGIDBODY_H
#define RIGIDBODY_H
#include "../core/component.hpp"

enum ForceMode {
	Force,
	Impulse
};

class Rigidbody : public Component {
	static const glm::vec3 Gravity;
public:
	Rigidbody(std::unordered_map<std::type_index, std::unique_ptr<Component>>& collection, Transform& transform, Object* object);
	glm::vec3 velocity;

	void AddForce(glm::vec3 force, ForceMode forcemode = ForceMode::Force);
	void Update() override;
};

#endif
#ifndef RIGIDBODY_H
#define RIGIDBODY_H
#include "../core/component.hpp"

class Rigidbody : public Component {
	static const glm::vec3 Gravity;
public:
	Rigidbody(std::unordered_map<std::type_index, std::unique_ptr<Component>>& collection, Transform& transform);
	glm::vec3 velocity;
	void Update() override;
};

#endif
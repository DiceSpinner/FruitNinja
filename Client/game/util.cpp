#include "util.hpp"
#include "rendering/camera.hpp"

bool isCursorInContact(Transform transform, float radius) {
	glm::vec3 ray = Cursor::getCursorRay();
	glm::vec3 oc = Camera::main->transform.position() - transform.position();
	float b = 2 * glm::dot(ray, oc);
	glm::vec3 scale = transform.scale();
	float c = glm::dot(oc, oc) - glm::pow(glm::max(scale.x, scale.y) * radius, 2);

	return (glm::pow(b, 2) - 4 * c) > 0;
}
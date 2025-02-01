#include <algorithm>
#include "camera.hpp"
#include "../state/window.hpp"

using namespace std;
Camera* Camera::main = nullptr;
vector<Camera*>* Camera::cameras = new vector<Camera*>();

Camera::Camera(std::unordered_map<std::type_index, std::unique_ptr<Component>>& components, Transform& transform, Object* object, float near, float far)
	: Component(components, transform, object), near(near), far(far), isOrtho(false)
{
	if (SCR_HEIGHT > 0) {
		perspective = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, near, far);
		float halfWidth = SCR_WIDTH / 2.0f;
		float halfHeight = SCR_HEIGHT / 2.0f;
		ortho = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight);
	}
	if (!main) {
		main = this;
	}
}

void Camera::SetPerspective(float near, float far) {
	this->near = near;
	this->far = far;
	if (SCR_HEIGHT > 0) {
		perspective = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, near, far);
		float halfWidth = SCR_WIDTH / 2.0f;
		float halfHeight = SCR_HEIGHT / 2.0f;
		ortho = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight);
	}
}

glm::mat4 Camera::Perspective() const {
	return perspective;
}

glm::mat4 Camera::Ortho() const {
	return ortho;
}

glm::mat4 Camera::Projection() const {
	return isOrtho ? ortho : perspective;
}

glm::mat4 Camera::View() const {
	return glm::lookAt(transform.position(), transform.position() + transform.forward(), transform.up());
}

float Camera::ComputerNormalizedZ(float viewZ) const {
	float top = 2 * near * far - viewZ * (far + near);
	float bot = -viewZ * (far - near);
	return top / bot;
}

void Camera::OnEnabled() {
	cameras->push_back(this);
}

void Camera::OnDisabled() {
	if (main == this) {
		main = nullptr; 
	}
	erase(*cameras, this);
}
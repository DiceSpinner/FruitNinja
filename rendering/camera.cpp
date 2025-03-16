#include <algorithm>
#include "camera.hpp"
#include "state/window.hpp"

using namespace std;
Camera* Camera::main = nullptr;
vector<Camera*>* Camera::cameras = new vector<Camera*>();

Camera::Camera(unordered_map<type_index, vector<unique_ptr<Component>>>& components, Transform& transform, Object* object, float nearClipPlane, float farClipPlane)
	: Component(components, transform, object), nearClipPlane(nearClipPlane), farClipPlane(farClipPlane), isOrtho(false), width(20)
{
	if (SCR_HEIGHT > 0) {
		perspective = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, nearClipPlane, farClipPlane);
		float halfWidth = SCR_WIDTH / 2.0f;
		float halfHeight = SCR_HEIGHT / 2.0f;
		ortho = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearClipPlane, farClipPlane);
	}
	if (!main) {
		main = this;
	}
}

void Camera::SetPerspective(float nearClipPlane, float farClipPlane) {
	this->nearClipPlane = nearClipPlane;
	this->farClipPlane = farClipPlane;
	if (SCR_HEIGHT > 0) {
		perspective = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, nearClipPlane, farClipPlane);
		float halfWidth = width / 2.0f;
		float halfHeight = (float)SCR_HEIGHT / SCR_WIDTH * halfWidth;
		ortho = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearClipPlane, farClipPlane);
	}
}

void Camera::SetOrthoWidth(float width) {
	this->width = width;
	float halfWidth = width / 2;
	float halfHeight = (float)SCR_HEIGHT / SCR_WIDTH * halfWidth;
	ortho = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight);
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
	float top = 2 * nearClipPlane * farClipPlane - viewZ * (farClipPlane + nearClipPlane);
	float bot = -viewZ * (farClipPlane - nearClipPlane);
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
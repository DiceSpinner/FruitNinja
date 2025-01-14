#include "object.hpp"
#include "glm/ext.hpp"

using namespace std;

Object::Object(shared_ptr<Model>& model) : model(model), transform(), components(), drawOverlay(false), enabled(true) {}

void Object::Draw(Shader& shader) const {
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "transform"), 1, GL_FALSE, glm::value_ptr(transform.matrix));
	if (drawOverlay) {
		glClear(GL_DEPTH_BUFFER_BIT);
	}
	model->Draw(shader);
}

void Object::Update() {
	for (auto& pair : components) {
		pair.second->Update();
	}
}

void Object::FixedUpdate() {
	for (auto& pair : components) {
		pair.second->FixedUpdate();
	}
}
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "renderer.hpp"

using namespace std;

static vector<Renderer*>* activeRenderers = new vector<Renderer*>();

void Renderer::DrawObjects(Shader& shader) {
	for (auto renderer : *activeRenderers) {
		renderer->Draw(shader);
	}
}

Renderer::Renderer(unordered_map<type_index, vector<unique_ptr<Component>>>& components, Transform& transform, Object* object,  shared_ptr<Model>& model)
	: Component(components, transform, object), model(model), drawOverlay(false)
{

}

void Renderer::Draw(Shader& shader) const {
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "transform"), 1, GL_FALSE, glm::value_ptr(transform.matrix));
	if (drawOverlay) {
		glClear(GL_DEPTH_BUFFER_BIT);
	}
	model->Draw(shader);
}

void Renderer::OnEnabled() {
	activeRenderers->push_back(this);
}

void Renderer::OnDisabled() {
	erase(*activeRenderers, this);
}
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <algorithm>
#include "renderer.hpp"

using namespace std;

static vector<Renderer*>* activeRenderers = new vector<Renderer*>();
GLuint Renderer::white(0);

void Renderer::DrawObjects(Shader& shader, Shader& outlineShader) {
	std::sort(activeRenderers->begin(), activeRenderers->end(), [](Renderer* a, Renderer* b) { return a->renderOrder < b->renderOrder; });
	for (auto renderer : *activeRenderers) {
		renderer->Draw(shader, outlineShader);
	}
}

Renderer::Renderer(unordered_map<type_index, vector<unique_ptr<Component>>>& components, Transform& transform, Object* object,  shared_ptr<Model>& model)
	: Component(components, transform, object), model(model), drawOverlay(false), drawOutline(false), outlineColor(1, 1, 1, 1), color(1, 1, 1, 1)
{

}

void Renderer::Draw(Shader& shader, Shader& outlineShader) const {
	if (drawOutline) {
		glClearStencil(0);
		glClear(GL_STENCIL_BUFFER_BIT);
		glStencilMask(0xFF);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
	}

	shader.SetVec4("color", color);
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "transform"), 1, GL_FALSE, glm::value_ptr(transform.matrix));
	if (drawOverlay) {
		glClear(GL_DEPTH_BUFFER_BIT);
	}
	model->Draw(shader);

	if (drawOutline) {
		outlineShader.Use();
		outlineShader.SetVec4("color", outlineColor);
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glUniformMatrix4fv(glGetUniformLocation(outlineShader.ID, "transform"), 1, GL_FALSE, glm::value_ptr(glm::scale(transform.matrix, 1.1f * glm::vec3(1, 1, 1))));
		model->Draw(outlineShader);
		shader.Use();
		glStencilFunc(GL_ALWAYS, 0, 0xFF);
	}
}

void Renderer::OnEnabled() {
	activeRenderers->push_back(this);
}

void Renderer::OnDisabled() {
	erase(*activeRenderers, this);
}
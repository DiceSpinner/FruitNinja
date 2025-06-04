#ifndef RENDERER_H
#define RENDERER_H
#include "rendering/model.hpp"
#include "infrastructure/object.hpp"
class Renderer : public Component {
private:
	std::shared_ptr<Model> model;
public:
	static GLuint white;

	bool drawOverlay;
	bool drawOutline;
	glm::vec4 color;
	glm::vec4 outlineColor;

	static void DrawObjects(Shader& shader, Shader& outlineShader);

	Renderer(std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& components, Transform& transform, Object* object, std::shared_ptr<Model>& model);
	void Draw(Shader& shader, Shader& outlineShader) const;
	void OnEnabled() override;
	void OnDisabled() override;
};

#endif
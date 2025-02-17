#ifndef RENDERER_H
#define RENDERER_H
#include "../rendering/model.hpp"
#include "../core/component.hpp"
class Renderer : public Component {
private:
	std::shared_ptr<Model> model;
public:
	bool drawOverlay;

	static void DrawObjects(Shader& shader);

	Renderer(std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& components, Transform& transform, Object* object, std::shared_ptr<Model>& model);
	void Draw(Shader& shader) const;
	void OnEnabled() override;
	void OnDisabled() override;
};

#endif
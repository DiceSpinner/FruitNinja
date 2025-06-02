#ifndef RENDER_CONTEXT
#define RENDER_CONTEXT
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

class RenderContext {
private:
	glm::uvec2 dimension;
	GLFWwindow* handle = {};
public:
	static RenderContext* Context;

	RenderContext(glm::uvec2 dimension);
	RenderContext(const RenderContext&) = delete;
	RenderContext(RenderContext&&) = delete;
	RenderContext& operator = (const RenderContext&) = delete;
	RenderContext& operator = (RenderContext&&) = delete;
	~RenderContext();

	bool Good() const;
	glm::uvec2 Dimension() const;
	void SetDimension(glm::uvec2 dimension);
	GLFWwindow* Window() const;
};

#endif
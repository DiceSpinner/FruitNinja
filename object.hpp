#ifndef OBJECT_H
#define OBJECT_H
#include<memory>
#include "model.hpp"

class Object {
private:
	std::shared_ptr<Model> model;
	bool alive;
public:
	glm::mat4 transform;

	Object(Model& model);

	glm::vec3 position() const;
	glm::vec3 up() const;
	glm::vec3 right() const;
	glm::vec3 forward() const;
	glm::mat4 rotation() const;
	glm::vec3 scale() const;
	void draw(Shader& shader) const;
	bool isAlive() const;

	void setPosition(glm::vec3 position);
	void setRotation(glm::mat4 newRotation);
	void setScale(glm::vec3 scale);
	void setForward(glm::vec3 direction);
	void setRight(glm::vec3 direction);
	void setUp(glm::vec3 direction);
	virtual void update();
	void destroy();
};
#endif
#include "Fruit.hpp"
#include "state/time.hpp"
#include "glm/ext.hpp"

using namespace std;
Fruit::Fruit(Model& model) : Object(model) {
}

void Fruit::update() {
	setRotation(glm::rotate(glm::mat4(1), time(), glm::vec3(0, 1, 0)));
}
#include <iostream>
#include "fruit.hpp"
#include "../state/cursor.hpp"
#include "../state/camera.hpp"
#include "../state/score.hpp"
#include "../state/new_objects.hpp"
#include "../core/object.hpp"
#include "fruitslice.hpp"
#include "rigidbody.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace std;

Fruit::Fruit(unordered_map<type_index, unique_ptr<Component>>& collection, Transform& transform, Object* object, 
	float radius, int score, shared_ptr<Model> slice1, shared_ptr<Model> slice2) :
	Component(collection, transform, object), radius(radius), score(score), slice1(slice1), slice2(slice2)
{

}

bool Fruit::CursorInContact() { // Sphere collision check
	glm::vec3 ray = getCursorRay();
	glm::vec3 oc = GameState::cameraPos - transform.position();
	float b = 2 * glm::dot(ray, oc);
	glm::vec3 scale = transform.scale();
	float c = glm::dot(oc, oc) - glm::pow(glm::max(scale.x, scale.y) * radius, 2);

	return (glm::pow(b, 2) - 4 * c) > 0;
}

void Fruit::Update() {
	
	glm::vec2 cursorDirection = getCursorPosDelta();

	if (!GameState::mouseClicked || glm::length(cursorDirection) == 0) { return; }

	if (CursorInContact()) {
		cout << "Fruit Sliced\n";
		GameState::score += this->score;
		cout << "Score " << GameState::score << "\n";
		// object->Destroy();

		shared_ptr<Object> slice1 = make_shared<Object>(this->slice1);
		shared_ptr<Object> slice2 = make_shared<Object>(this->slice2);
	    slice1->AddComponent<FruitSlice>();
		auto r1 = slice1->AddComponent<Rigidbody>();
		slice2->AddComponent<FruitSlice>();
		auto r2 = slice2->AddComponent<Rigidbody>();

		glm::vec3 sliceDirection = glm::vec3(cursorDirection, 0);
		glm::vec3 up = glm::cross(glm::vec3(0, 0, 1), sliceDirection);
		slice1->transform.SetUp(up);
		r1->AddForce(300.0f * up, ForceMode::Impulse);
		slice2->transform.SetUp(up);
		r2->AddForce(-300.0f * up, ForceMode::Impulse);
		GameState::newObjects.push(slice1);
		GameState::newObjects.push(slice2);
	}
}
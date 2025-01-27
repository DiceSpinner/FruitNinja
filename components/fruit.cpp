#include <iostream>
#include "fruit.hpp"
#include "../settings/fruitspawn.hpp"
#include "../state/cursor.hpp"
#include "../state/camera.hpp"
#include "../state/state.hpp"
#include "../state/new_objects.hpp"
#include "../core/object.hpp"
#include "fruitslice.hpp"
#include "rigidbody.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace std;

Fruit::Fruit(unordered_map<type_index, unique_ptr<Component>>& collection, Transform& transform, Object* object,
	float radius, int score, shared_ptr<Model> slice1, shared_ptr<Model> slice2) :
	Component(collection, transform, object), radius(radius), reward(score), slice1(slice1), slice2(slice2)
{

}

bool Fruit::CursorInContact() { // Sphere collision check
	glm::vec3 ray = getCursorRay();
	glm::vec3 oc = Game::cameraPos - transform.position();
	float b = 2 * glm::dot(ray, oc);
	glm::vec3 scale = transform.scale();
	float c = glm::dot(oc, oc) - glm::pow(glm::max(scale.x, scale.y) * radius, 2);

	return (glm::pow(b, 2) - 4 * c) > 0;
}

void Fruit::Update() {
	glm::vec2 cursorDirection = getCursorPosDelta();
	if (transform.position().y <= FRUIT_KILL_HEIGHT) {
		if (this->reward > 0) {
			Game::misses++;
		}
		object->enabled = false;
		return;
	}
	if (!Game::mouseClicked || glm::length(cursorDirection) == 0) { return; }

	if (CursorInContact()) {
		// cout << "Fruit Sliced\n";
		if (Game::state == State::GAME) {
			Game::score += this->reward;
			int quotient = Game::score / 50;
			if (quotient > Game::recovery) {
				Game::misses = glm::max(Game::misses - (quotient - Game::recovery), 0);
				Game::recovery = quotient;
			}
		}
		
		// cout << "Score " << Game::score << "\n";
		object->enabled = false;

		shared_ptr<Object> slice1 = make_shared<Object>(this->slice1);
		shared_ptr<Object> slice2 = make_shared<Object>(this->slice2);
	    slice1->AddComponent<FruitSlice>();
		auto r1 = slice1->AddComponent<Rigidbody>();
		slice2->AddComponent<FruitSlice>();
		auto r2 = slice2->AddComponent<Rigidbody>();

		glm::vec3 sliceDirection = glm::vec3(cursorDirection, 0);
		glm::vec3 up = glm::normalize(glm::cross(glm::vec3(0, 0, 1), sliceDirection));
		
		if (glm::dot(up, glm::vec3(0, 1, 0)) < 0) {
			up = -up;
		}

		Rigidbody* rb = GetComponent<Rigidbody>();
		slice1->transform.SetPosition(transform.position());
		// slice1->transform.SetForward(transform.forward());
		slice1->transform.SetUp(up);
		r1->velocity = rb->velocity;
		r1->AddForce(FRUIT_SLICE_FORCE * up, ForceMode::Impulse);
		r1->AddRelativeTorque(-180.0f * glm::vec3(1, 0, 0), ForceMode::Impulse);

		slice2->transform.SetPosition(transform.position());
		// slice2->transform.SetForward(transform.forward());
		slice2->transform.SetUp(up);
		r2->velocity = rb->velocity;
		r2->AddForce(-FRUIT_SLICE_FORCE * up, ForceMode::Impulse);
		r2->AddRelativeTorque(180.0f * glm::vec3(1, 0, 0), ForceMode::Impulse);
		
		Game::newObjects.push(slice1);
		Game::newObjects.push(slice2);
	}
}
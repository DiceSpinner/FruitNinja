#include <iostream>
#include "fruit.hpp"
#include "../audio/audiosource_pool.hpp"
#include "../physics/rigidbody.hpp"
#include "../rendering/renderer.hpp"
#include "../rendering/camera.hpp"
#include "../settings/fruitspawn.hpp"
#include "../state/cursor.hpp"
#include "../state/state.hpp"
#include "../core/object.hpp"
#include "fruitslice.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace std;

Fruit::Fruit(std::unordered_map<std::type_index, std::unique_ptr<Component>>& components, Transform& transform, Object* object,
	float radius, int score, shared_ptr<Model> slice1, shared_ptr<Model> slice2, shared_ptr<AudioClip> clipOnSliced, shared_ptr<AudioClip> clipOnMissed) :
	Component(components, transform, object), radius(radius), reward(score), slice1(slice1), slice2(slice2), clipOnSliced(clipOnSliced), clipOnMissed(clipOnMissed)
{

}

bool Fruit::CursorInContact() { // Sphere collision check
	glm::vec3 ray = getCursorRay();
	glm::vec3 oc = Camera::main->transform.position() - transform.position();
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
		if (clipOnMissed) {
			auto audioSourceObj = acquireAudioSource();
			if (audioSourceObj) {
				auto source = audioSourceObj->GetComponent<AudioSource>();
				source->SetAudioClip(clipOnMissed);
				source->Play();
			}
		}
		object->SetEnable(false);
		return;
	}
	if (!Game::mouseClicked || glm::length(cursorDirection) == 0) { return; }

	if (CursorInContact()) {
		// cout << "Fruit Sliced\n";
		if (Game::state == State::GAME) {
			Game::score += this->reward;
			int quotient = Game::score / 50;
			if (quotient > Game::recovery) {
				auto before = Game::misses;
				Game::misses = glm::max(Game::misses - (quotient - Game::recovery), 0);
				Game::recovery = quotient;
				if (before > Game::misses) {
					Game::recentlyRecovered = true;
				}
			}
		}
		
		// cout << "Score " << Game::score << "\n";
		object->SetEnable(false);

		shared_ptr<Object> slice1 = Object::Create();
		slice1->AddComponent<Renderer>(this->slice1);
		slice1->AddComponent<FruitSlice>();
		auto r1 = slice1->AddComponent<Rigidbody>();

		shared_ptr<Object> slice2 = Object::Create();
		slice2->AddComponent<Renderer>(this->slice2);
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

		if (clipOnSliced) {
			auto audioSourceObj = acquireAudioSource();
			if (audioSourceObj) {
				auto source = audioSourceObj->GetComponent<AudioSource>();
				source->SetAudioClip(clipOnSliced);
				source->Play();
			}
		}
	}
}
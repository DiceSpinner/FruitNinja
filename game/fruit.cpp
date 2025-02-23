#include <iostream>
#include "fruit.hpp"
#include "../audio/audiosource_pool.hpp"
#include "../physics/rigidbody.hpp"
#include "../rendering/renderer.hpp"
#include "../rendering/camera.hpp"
#include "../rendering/particle_system.hpp"
#include "../settings/fruitspawn.hpp"
#include "../state/state.hpp"
#include "../core/object.hpp"
#include "util.hpp"
#include "fruitslice.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace std;

Fruit::Fruit(
	unordered_map<type_index, vector<unique_ptr<Component>>>& components, Transform& transform, Object* object,
	float radius, int score, 
	ObjectPool<Object>& particlePool, 
	shared_ptr<Model> slice1, 
	shared_ptr<Model> slice2, 
	shared_ptr<AudioClip> clipOnSliced, 
	shared_ptr<AudioClip> clipOnMissed) :
	Component(components, transform, object), radius(radius), reward(score), color(1, 1, 1, 1),
	particlePool(particlePool), slice1(slice1), slice2(slice2), clipOnSliced(clipOnSliced), 
	clipOnMissed(clipOnMissed), slicedParticleTexture(0)
{

}

void Fruit::PlayVFX() const {
	if (!slicedParticleTexture) {
		return;
	}
	shared_ptr<Object> particleSystemObj = particlePool.Acquire();
	particleSystemObj->SetEnable(true);
	particleSystemObj->transform.SetPosition(transform.position());
	ParticleSystem* particleSystem = particleSystemObj->GetComponent<ParticleSystem>();
	particleSystem->texture = slicedParticleTexture;
	particleSystem->color = color;
}

void Fruit::Update() {
	if (Game::state == State::EXPLOSION) {
		return;
	}
	if (Game::state == State::SCORE && reward > 0) {
		object->SetEnable(false);
		return;
	}

	glm::vec2 cursorDirection = getCursorPosDelta();
	if (transform.position().y <= FRUIT_KILL_HEIGHT) {
		if (this->reward > 0) {
			Game::misses++;
		}
		if (Game::state == State::GAME && clipOnMissed) {
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

	if (isCursorInContact(transform, radius)) {
		// cout << "Fruit Sliced\n";
		PlayVFX();
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

		if (slice1 && slice2) {
			shared_ptr<Object> topSlice = Object::Create();
			Renderer* renderer = topSlice->AddComponent<Renderer>(this->slice1);
			// renderer->drawOverlay = true;
			topSlice->AddComponent<FruitSlice>();
			auto r1 = topSlice->AddComponent<Rigidbody>();

			shared_ptr<Object> bottomSlice = Object::Create();
			renderer = bottomSlice->AddComponent<Renderer>(this->slice2);
			// renderer->drawOverlay = true;
			bottomSlice->AddComponent<FruitSlice>();
			auto r2 = bottomSlice->AddComponent<Rigidbody>();

			glm::vec3 sliceDirection = glm::vec3(cursorDirection, 0);
			glm::vec3 up = glm::normalize(glm::cross(glm::vec3(0, 0, 1), sliceDirection));

			if (glm::dot(up, glm::vec3(0, 1, 0)) < 0) {
				up = -up;
			}

			Rigidbody* rb = GetComponent<Rigidbody>();
			topSlice->transform.SetPosition(transform.position());
			// slice1->transform.SetForward(transform.forward());
			topSlice->transform.SetUp(up);
			r1->velocity = rb->velocity;
			r1->AddForce(FRUIT_SLICE_FORCE * up, ForceMode::Impulse);
			r1->AddRelativeTorque(-180.0f * glm::vec3(1, 0, 0), ForceMode::Impulse);

			bottomSlice->transform.SetPosition(transform.position());
			// slice2->transform.SetForward(transform.forward());
			bottomSlice->transform.SetUp(up);
			r2->velocity = rb->velocity;
			r2->AddForce(-FRUIT_SLICE_FORCE * up, ForceMode::Impulse);
			r2->AddRelativeTorque(180.0f * glm::vec3(1, 0, 0), ForceMode::Impulse);
		}

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
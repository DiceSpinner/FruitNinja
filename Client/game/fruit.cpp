#include <iostream>
#include "fruit.hpp"
#include "audio/audiosource_pool.hpp"
#include "physics/rigidbody.hpp"
#include "rendering/renderer.hpp"
#include "rendering/camera.hpp"
#include "rendering/particle_system.hpp"
#include "infrastructure/object.hpp"
#include "util.hpp"
#include "fruitslice.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace std;

Fruit::Fruit
(
	unordered_map<type_index, vector<unique_ptr<Component>>>& components, Transform& transform, Object* object,
	float radius, int score, float sliceForce, FruitChannel& channel, const FruitAsset& asset
) :
	Component(components, transform, object), 
	radius(radius), reward(score), sliceForce(sliceForce), color(1, 1, 1, 1), 
	channel(channel), assets(asset), 
	slicedParticleTexture(0)
{

}

void Fruit::PlayVFX() const {
	if (!slicedParticleTexture) {
		return;
	}
	shared_ptr<Object> particleSystemObj = channel.particleSystemPool->Acquire();
	particleSystemObj->SetEnable(true);
	particleSystemObj->transform.SetPosition(transform.position());
	ParticleSystem* particleSystem = particleSystemObj->GetComponent<ParticleSystem>();
	particleSystem->texture = slicedParticleTexture;
	particleSystem->color = color;
}

void Fruit::Update() {
	if (channel.disableNonUI && reward > 0) {
		object->SetEnable(false);
		return;
	}

	glm::vec2 cursorDirection = Cursor::getCursorPosDelta();
	if (transform.position().y <= channel.killHeight) {
		if (this->reward > 0) {
			channel.miss++;
		}
		if (reward > 0 && assets.clipOnMissed) {
			auto audioSourceObj = acquireAudioSource();
			if (audioSourceObj) {
				auto source = audioSourceObj->GetComponent<AudioSource>();
				source->SetAudioClip(assets.clipOnMissed);
				source->Play();
			}
		}
		object->SetEnable(false);
		return;
	}
	if (!Cursor::mouseClicked || glm::length(cursorDirection) == 0) { return; }

	if (channel.enableSlicing && isCursorInContact(transform, radius)) {
		// cout << "Fruit Sliced\n";
		PlayVFX();
		channel.score += this->reward;
		int quotient = channel.score / 50;
		if (quotient > channel.recovery) {
			auto before = channel.miss;
			channel.miss = glm::max(channel.miss - (quotient - channel.recovery), 0);
			channel.recovery = quotient;
			if (before > channel.miss) {
				channel.recentlyRecovered = true;
			}
		}
		
		// cout << "Score " << Game::score << "\n";
		object->SetEnable(false);

		if (assets.slice1 && assets.slice2) {
			shared_ptr<Object> topSlice = Object::Create();
			Renderer* renderer = topSlice->AddComponent<Renderer>(assets.slice1);
			renderer->drawOverlay = true;
			topSlice->AddComponent<FruitSlice>(channel, reward == 0);
			auto r1 = topSlice->AddComponent<Rigidbody>();

			shared_ptr<Object> bottomSlice = Object::Create();
			renderer = bottomSlice->AddComponent<Renderer>(assets.slice2);
			renderer->drawOverlay = true;
			bottomSlice->AddComponent<FruitSlice>(channel, reward == 0);
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
			r1->AddForce(sliceForce * up, ForceMode::Impulse);
			r1->AddRelativeTorque(-180.0f * glm::vec3(1, 0, 0), ForceMode::Impulse);

			bottomSlice->transform.SetPosition(transform.position());
			// slice2->transform.SetForward(transform.forward());
			bottomSlice->transform.SetUp(up);
			r2->velocity = rb->velocity;
			r2->AddForce(-sliceForce * up, ForceMode::Impulse);
			r2->AddRelativeTorque(180.0f * glm::vec3(1, 0, 0), ForceMode::Impulse);
		}

		if (assets.clipOnSliced) {
			auto audioSourceObj = acquireAudioSource();
			if (audioSourceObj) {
				auto source = audioSourceObj->GetComponent<AudioSource>();
				source->SetAudioClip(assets.clipOnSliced);
				source->Play();
			}
		}
	}
}
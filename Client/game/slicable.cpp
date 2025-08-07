#include "slicable.hpp"
#include "audio/audiosource_pool.hpp"
#include "physics/rigidbody.hpp"
#include "rendering/renderer.hpp"
#include "rendering/camera.hpp"
#include "rendering/particle_system.hpp"
#include "infrastructure/object.hpp"
#include "game/util.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include "debug/log.hpp"

using namespace std;

Sliced::Sliced(std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& components, Transform& transform, Object* object,
	const std::shared_ptr<SlicableControl>& control) : Component(components, transform, object),
	control(control)
{
	if (!control) {
		Debug::LogError("Slice component is not initialized with a control block");
	}
}

void Sliced::Update(const Clock& clock) {
	if (!control || transform.position().y <= control->killHeight || control->disableAll) {
		object->Detach();
	}
}

Slicable::Slicable
(
	std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& components, Transform& transform, Object* object,
	float radius, float sliceForce, const std::shared_ptr<SlicableControl>& control, const SlicableAsset& asset
) :
	Component(components, transform, object),
	radius(radius), sliceForce(sliceForce),
	control(control), asset(asset)
{
	if (!control) {
		Debug::LogError("Slicable not initialized with a control block");
	}
}

void Slicable::Slice(const Clock& clock, glm::vec3 up) {
	sliced = true;
	if (onSliced) {
		onSliced(transform);
	}

	if (asset.topSlice && asset.bottomSlice) {
		shared_ptr<Object> topSlice = object->Manager()->CreateObject();
		Renderer* renderer = topSlice->AddComponent<Renderer>(asset.topSlice);
		renderer->drawOverlay = true;
		topSlice->AddComponent<Sliced>(control);
		auto r1 = topSlice->AddComponent<Rigidbody>();

		shared_ptr<Object> bottomSlice = object->Manager()->CreateObject();
		renderer = bottomSlice->AddComponent<Renderer>(asset.bottomSlice);
		renderer->drawOverlay = true;
		bottomSlice->AddComponent<Sliced>(control);
		auto r2 = bottomSlice->AddComponent<Rigidbody>();

		Rigidbody* rb = GetComponent<Rigidbody>();
		topSlice->transform.SetPosition(transform.position());
		// slice1->transform.SetForward(transform.forward());
		topSlice->transform.SetUp(up);
		r1->velocity = rb->velocity;
		r1->AddForce(clock, sliceForce * up, ForceMode::Impulse);
		r1->AddRelativeTorque(clock, -180.0f * glm::vec3(1, 0, 0), ForceMode::Impulse);

		bottomSlice->transform.SetPosition(transform.position());
		// slice2->transform.SetForward(transform.forward());
		bottomSlice->transform.SetUp(up);
		r2->velocity = rb->velocity;
		r2->AddForce(clock, -sliceForce * up, ForceMode::Impulse);
		r2->AddRelativeTorque(clock, 180.0f * glm::vec3(1, 0, 0), ForceMode::Impulse);
	}

	if (asset.clipOnSliced) {
		auto audioSourceObj = acquireAudioSource();
		if (audioSourceObj) {
			auto source = audioSourceObj->GetComponent<AudioSource>();
			source->SetAudioClip(asset.clipOnSliced);
			source->Play();
		}
	}
	if (destroyOnSliced) {
		object->Detach();
	}
}

void Slicable::PlayParticleVFX() const {
	if (!particleTexture || !control || !control->particlePool) {
		return;
	}
	shared_ptr<Object> particleSystemObj = control->particlePool->Acquire();
	object->Manager()->Register(particleSystemObj);
	particleSystemObj->transform.SetPosition(transform.position());
	ParticleSystem* particleSystem = particleSystemObj->GetComponent<ParticleSystem>();
	particleSystem->texture = particleTexture;
	particleSystem->color = particleColor;
}

void Slicable::Update(const Clock& clock) {
	if (!control || control->disableAll) {
		object->Detach();
		return;
	}

	glm::vec2 cursorDirection = Cursor::getCursorPosDelta();
	if (transform.position().y <= control->killHeight) {
		if (onMissed) {
			onMissed();
		}
		if (asset.clipOnMissed) {
			auto audioSourceObj = acquireAudioSource();
			if (audioSourceObj) {
				auto source = audioSourceObj->GetComponent<AudioSource>();
				source->SetAudioClip(asset.clipOnMissed);
				source->Play();
			}
		}

		object->Detach();
		return;
	}
	if (sliced || control->disableSlicing || !Cursor::mouseClicked || glm::length(cursorDirection) == 0) { return; }

	if (isCursorInContact(transform, radius)) {
		// cout << "Fruit Sliced\n";
		PlayParticleVFX();
		
		glm::vec3 sliceDirection = glm::vec3(cursorDirection, 0);
		glm::vec3 up = glm::normalize(glm::cross(glm::vec3(0, 0, 1), sliceDirection));

		if (glm::dot(up, glm::vec3(0, 1, 0)) < 0) {
			up = -up;
		}
		Slice(clock, up);
	}
}
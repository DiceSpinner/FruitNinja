#include "bomb.hpp"
#include "util.hpp"
#include "infrastructure/object.hpp"

using namespace std;

Bomb::Bomb(unordered_map<type_index, vector<unique_ptr<Component>>>& collection, Transform& transform, Object* object, shared_ptr<AudioClip> explosionSFX, float radius, BombChannel& control)
	: Component(collection, transform, object), explosionSFX(explosionSFX), radius(radius), channel(control)
{
	
}

void Bomb::Update(Clock& clock) {
	if (channel.disableAll) {
		object->Detach();
		return;
	}
	if (!channel.enableSlicing) {
		return;
	}

	glm::vec2 cursorDirection = Cursor::getCursorPosDelta();
	if (transform.position().y <= channel.killHeight) {
		object->Detach();
		return;
	}

	if (!Cursor::mouseClicked || glm::length(cursorDirection) == 0) { return; }

	if (isCursorInContact(transform, radius)) {
		// cout << "Fruit Sliced\n";
		channel.bombHit = true;

		if (explosionSFX) {
			auto audioSourceObj = acquireAudioSource();
			if (audioSourceObj) {
				auto source = audioSourceObj->GetComponent<AudioSource>();
				source->SetAudioClip(explosionSFX);
				source->Play();
			}
		}

		channel.explosionPosition = transform.position();
	}
}
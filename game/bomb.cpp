#include "bomb.hpp"
#include "util.hpp"
#include "../core/object.hpp"
#include "../settings/fruitspawn.hpp"
#include "../state/state.hpp"

using namespace std;

Bomb::Bomb(unordered_map<type_index, vector<unique_ptr<Component>>>& collection, Transform& transform, Object* object, shared_ptr<AudioClip> explosionSFX, float radius)
	: Component(collection, transform, object), explosionSFX(explosionSFX), radius(radius)
{
	
}

void Bomb::Update() {
	if (Game::state == State::EXPLOSION) {
		return;
	}

	if (Game::state == State::SCORE) {
		object->SetEnable(false);
		return;
	}

	glm::vec2 cursorDirection = getCursorPosDelta();
	if (transform.position().y <= BOMB_KILL_HEIGHT) {
		object->SetEnable(false);
		return;
	}

	if (!Game::mouseClicked || glm::length(cursorDirection) == 0) { return; }

	if (Game::state == State::GAME && isCursorInContact(transform, radius)) {
		// cout << "Fruit Sliced\n";
		if (Game::state == State::GAME) {
			Game::bombHit = true;
		}

		if (explosionSFX) {
			auto audioSourceObj = acquireAudioSource();
			if (audioSourceObj) {
				auto source = audioSourceObj->GetComponent<AudioSource>();
				source->SetAudioClip(explosionSFX);
				source->Play();
			}
		}

		Game::explosionPosition = transform.position();
	}
}
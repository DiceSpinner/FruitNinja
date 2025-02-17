#include "fruitslice.hpp"
#include "../state/time.hpp"
#include "../state/state.hpp"
#include "../core/object.hpp"
#include "../settings/fruitspawn.hpp"

using namespace std;

FruitSlice::FruitSlice(unordered_map<type_index, vector<unique_ptr<Component>>>& components, Transform& transform, Object* object)
	: Component(components, transform, object) {
}

void FruitSlice::Update() {
	if (Game::state == State::SCORE) {
		object->SetEnable(false);
		return;
	}
	if (transform.position().y <= FRUIT_KILL_HEIGHT) {
		object->SetEnable(false);
	}
}
#include "fruitslice.hpp"
#include "../state/time.hpp"
#include "../core/object.hpp"

using namespace std;

FruitSlice::FruitSlice(unordered_map<type_index, vector<unique_ptr<Component>>>& components, Transform& transform, Object* object, FruitChannel& channel, bool isUI)
	: Component(components, transform, object), channel(channel), isUI(isUI) {
}

void FruitSlice::Update() {
	if (transform.position().y <= channel.killHeight || (channel.disableNonUI && !isUI)) {
		object->SetEnable(false);
	}
}
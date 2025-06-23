#include "fruitslice.hpp"
#include "infrastructure/object.hpp"

using namespace std;

FruitSlice::FruitSlice(unordered_map<type_index, vector<unique_ptr<Component>>>& components, Transform& transform, Object* object, FruitChannel& channel, bool isUI)
	: Component(components, transform, object), channel(channel), isUI(isUI) {
}

void FruitSlice::Update(const Clock& clock) {
	if (transform.position().y <= channel.killHeight || (channel.disableNonUI && !isUI)) {
		object->Detach();
	}
}
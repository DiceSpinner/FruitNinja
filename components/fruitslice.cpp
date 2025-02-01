#include "fruitslice.hpp"
#include "../state/time.hpp"
#include "../core/object.hpp"
#include "../settings/fruitspawn.hpp"

using namespace std;

FruitSlice::FruitSlice(std::unordered_map<std::type_index, std::unique_ptr<Component>>& components, Transform& transform, Object* object)
	: Component(components, transform, object) {
}

void FruitSlice::Update() {
	if (transform.position().y <= FRUIT_KILL_HEIGHT) {
		object->SetEnable(false);
	}
}
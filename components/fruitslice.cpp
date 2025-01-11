#include "fruitslice.hpp"
#include "../state/time.hpp"
#include "../core/object.hpp"
#include "../settings/fruitspawn.hpp"

using namespace std;

FruitSlice::FruitSlice(unordered_map<type_index, unique_ptr<Component>>& collection, Transform& transform, Object* object) 
	: Component(collection, transform, object) {
}

void FruitSlice::Update() {
	if (transform.position().y <= FRUIT_KILL_HEIGHT) {
		object->enabled = false;
	}
}
#include "fruitslice.hpp"
#include "../state/time.hpp"
#include "../core/object.hpp"

using namespace std;

FruitSlice::FruitSlice(unordered_map<type_index, unique_ptr<Component>>& collection, Transform& transform, Object* object) 
	: Component(collection, transform, object), timeCounter(0) {
}

void FruitSlice::Update() {
	timeCounter += deltaTime();
	if (timeCounter >= 5) {
		object->Destroy();
	}
}
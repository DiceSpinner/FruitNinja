#include "component.hpp"

using namespace std;

Component::Component(unordered_map<std::type_index, std::unique_ptr<Component>>& collection, Transform& transform, Object* object) :
	componentMap(collection),
	transform(transform) ,
	object(object)
{
}

void Component::Update() {}
void Component::Initialize() {}
void Component::FixedUpdate() {}
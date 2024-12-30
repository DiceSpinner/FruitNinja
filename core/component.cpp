#include "component.hpp"

using namespace std;

Component::Component(unordered_map<std::type_index, std::unique_ptr<Component>>& collection, Transform& transform) :
	componentMap(collection),
	transform(transform) 
{
}

void Component::Update() {}
void Component::Initialize() {}
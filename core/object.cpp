#include <list>
#include <iterator>
#include "object.hpp"
#include "glm/ext.hpp"

using namespace std;

unordered_set<shared_ptr<Object>> Object::newObjectSet;
static list<shared_ptr<Object>> objectList;

shared_ptr<Object> Object::Create() {
	return *newObjectSet.emplace(new Object()).first;
}

void Object::ActivateNewlyEnabledObjects() {
	move(newObjectSet.begin(), newObjectSet.end(), back_inserter(objectList));
	newObjectSet.clear();
}

void Object::ExecuteEarlyFixedUpdate() {
	for (auto i = objectList.begin(); i != objectList.end();) {
		auto& obj = *i;
		for (auto& pair : obj->components) {
			pair.second->EarlyFixedUpdate();
		}
		if (obj->IsActive()) {
			i++;
		}
		else {
			i = objectList.erase(i);
		}
	}
}

void Object::ExecuteFixedUpdate() {
	for (auto i = objectList.begin(); i != objectList.end();) {
		auto& obj = *i;
		for (auto& pair : obj->components) {
			pair.second->FixedUpdate();
		}
		if (obj->IsActive()) {
			i++;
		}
		else {
			i = objectList.erase(i);
		}
	}
}

void Object::ExecuteUpdate() {
	for (auto i = objectList.begin(); i != objectList.end();) {
		auto& obj = *i;
		for (auto& pair : obj->components) {
			pair.second->Update();
		}
		if (obj->IsActive()) {
			i++;
		}
		else {
			i = objectList.erase(i);
		}
	}
}

Object::Object() :transform(), components(), enabled(true) {}

void Object::SetEnable(bool value) {
	if (enabled == value) return;
	if (!enabled) {
		newObjectSet.emplace(enable_shared_from_this::shared_from_this());
		for (auto& item : components) {
			item.second->OnEnabled();
		}
	}
	else {
		newObjectSet.erase(enable_shared_from_this::shared_from_this());
		for (auto& item : components) {
			item.second->OnDisabled();
		}
	}
	enabled = value;
}

bool Object::IsActive() const {
	return enabled;
}

Object::~Object() {
	for (auto& item : components) {
		item.second->OnDisabled();
		item.second->OnDestroyed();
	}
}
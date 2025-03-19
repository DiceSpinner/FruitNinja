#include <list>
#include <iterator>
#include "object.hpp"
#include "glm/ext.hpp"

using namespace std;

unordered_set<shared_ptr<Object>> Object::newObjectSet;
static list<shared_ptr<Object>> objectList;

void Object::ActivateNewlyEnabledObjects() {
	move(newObjectSet.begin(), newObjectSet.end(), back_inserter(objectList));
	newObjectSet.clear();
}

void Object::ExecuteEarlyFixedUpdate() {
	for (auto i = objectList.begin(); i != objectList.end();) {
		auto& obj = *i;
		for (auto& pair : obj->components) {
			for (auto& cmp : pair.second) {
				cmp->EarlyFixedUpdate();
			}
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
			for (auto& cmp : pair.second) {
				cmp->FixedUpdate();
			}
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
			for (auto& cmp : pair.second) {
				cmp->Update();
			}
		}
		if (obj->IsActive()) {
			i++;
		}
		else {
			i = objectList.erase(i);
		}
	}
}

shared_ptr<Object> Object::Create(bool isEnabled) {
	auto ptr = shared_ptr<Object>(new Object(isEnabled));
	if (isEnabled) {
		newObjectSet.insert(ptr);
	}
	return ptr;
}

Object::Object() : transform(), components(), enabled(false) { }

Object::Object(bool isEnabled) : transform(), components(), enabled(isEnabled) { }

void Object::SetEnable(bool value) {
	if (enabled == value) return;
	if (!enabled) {
		newObjectSet.emplace(enable_shared_from_this::shared_from_this());
		for (auto& item : components) {
			for (auto& cmp : item.second) {
				cmp->OnEnabled();
			}
		}
	}
	else {
		newObjectSet.erase(enable_shared_from_this::shared_from_this());
		for (auto& item : components) {
			for (auto& cmp : item.second) {
				cmp->OnDisabled();
			}
		}
	}
	enabled = value;
}

bool Object::IsActive() const {
	return enabled;
}
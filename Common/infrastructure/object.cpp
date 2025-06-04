#include <list>
#include <iterator>
#include <iostream>
#include "object.hpp"
#include "glm/ext.hpp"

using namespace std;

ObjectManager::ObjectManager() {}

void ObjectManager::ExecuteEarlyFixedUpdate(Clock& clock) {
	for (auto i = updateList.begin(); i != updateList.end();) {
		auto& obj = *(i++);
		obj->isUpdating = true;
		for (auto& pair : obj->components) {
			for (auto& cmp : pair.second) {
				cmp->EarlyFixedUpdate(clock);
			}
		}
		obj->isUpdating = false;
		if (obj->signaledDetachment) {
			obj->signaledDetachment = false;
			Unregister(obj);
		}
	}
}

void ObjectManager::ExecuteFixedUpdate(Clock& clock) {
	for (auto i = updateList.begin(); i != updateList.end();) {
		auto& obj = *(i++);
		obj->isUpdating = true;
		for (auto& pair : obj->components) {
			for (auto& cmp : pair.second) {
				cmp->FixedUpdate(clock);
			}
		}
		obj->isUpdating = false;
		if (obj->signaledDetachment) {
			obj->signaledDetachment = false;
			Unregister(obj);
		}
	}
}

void ObjectManager::ExecuteUpdate(Clock& clock) {
	for (auto i = updateList.begin(); i != updateList.end();) {
		auto& obj = *(i++);
		obj->isUpdating = true;
		for (auto& pair : obj->components) {
			for (auto& cmp : pair.second) {
				cmp->Update(clock);
			}
		}
		obj->isUpdating = false;
		if (obj->signaledDetachment) {
			obj->signaledDetachment = false;
			Unregister(obj);
		}
	}
}

void ObjectManager::Tick(Clock& clock) {
	while (clock.ShouldUpdatePhysics()) {
		ExecuteEarlyFixedUpdate(clock);
		ExecuteFixedUpdate(clock);
	}

	ExecuteUpdate(clock);
}

std::shared_ptr<Object> ObjectManager::CreateObject() {
	std::shared_ptr<Object> obj = std::make_shared<Object>();
	activeObjects.insert({ obj.get(), obj });
	obj->pointer = updateList.insert(updateList.end(), obj.get());
	obj->manager = this;
	return obj;
}

void ObjectManager::Register(const std::shared_ptr<Object>& obj) {
	if (obj->manager) return;
	obj->manager = this;
	obj->EnableAllComponents();
	activeObjects.insert({obj.get(), obj});
	obj->pointer = updateList.insert(updateList.end(), obj.get());
}

void ObjectManager::Unregister(Object* obj) {
	if (!obj || obj->manager != this) return;
	obj->manager = nullptr;
	obj->DisableAllComponents();
	updateList.erase(obj->pointer);
	activeObjects.erase(obj);
}

ObjectManager::~ObjectManager() {
	for (auto& cmp : updateList) {
		cmp->manager = nullptr;
		cmp->DisableAllComponents();
	}
	activeObjects.clear();
}

Object::Object() {}
Object::~Object() { }

bool Object::IsActive() const {
	return manager;
}

void Object::Detach() {
	if (!manager) return;
	if (isUpdating) {
		signaledDetachment = true;
		return;
	}
	manager->Unregister(this);
}

void Object::DisableAllComponents() const {
	for (auto& [type, group] : components) {
		for (auto& cmp : group) {
			cmp->OnDisabled();
		}
	}
}

void Object::EnableAllComponents() const {
	for (auto& [type, group] : components) {
		for (auto& cmp : group) {
			cmp->OnEnabled();
		}
	}
}

ObjectManager* Object::Manager() const { return manager; }

Component::Component(std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& collection, Transform& transform, Object* object) :
	componentMap(collection),
	transform(transform),
	object(object)
{

}

void Component::Update(Clock& clock) {}
void Component::Initialize() {}
void Component::EarlyFixedUpdate(Clock& clock) {}
void Component::FixedUpdate(Clock& clock) {}
void Component::OnEnabled() {}
void Component::OnDisabled() {}
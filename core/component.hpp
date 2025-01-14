#ifndef COMPONENT_H
#define COMPONENT_H
#include <string>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include "transform.hpp"

class Object;

class Component {
private:
	std::unordered_map<std::type_index, std::unique_ptr<Component>>& componentMap;
public:
	Object* object;
	Transform& transform;
	Component(std::unordered_map<std::type_index, std::unique_ptr<Component>>& collection, Transform& transform, Object* object);
	void virtual Update();
	void virtual Initialize();
	void virtual FixedUpdate();

	template<typename T>
	T* GetComponent() {
		auto item = componentMap.find(std::type_index(typeid(T)));
		if (item != componentMap.end()) {
			return (T*)item->second.get();
		}
		return nullptr;
	}
};
#endif
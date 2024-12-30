#ifndef COMPONENT_H
#define COMPONENT_H
#include <string>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include "transform.hpp"

class Component {
private:
	std::unordered_map<std::type_index, std::unique_ptr<Component>>& componentMap;
public:
	Transform& transform;
	Component(std::unordered_map<std::type_index, std::unique_ptr<Component>>& collection, Transform& transform);
	void virtual Update();
	void virtual Initialize();

	template<typename T>
	T* GetComponent(T componentType) {
		auto item = componentMap.find(std::type_index(typeid(T)));
		if (item != componentMap.end()) {
			return item->second.get();
		}
		return nullptr;
	}
};
#endif
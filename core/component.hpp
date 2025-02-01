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
	void virtual OnEnabled();
	void virtual OnDisabled();
	void virtual OnDestroyed();

	template<typename T>
	T* GetComponent() {
		auto item = componentMap.find(std::type_index(typeid(T)));
		if (item != componentMap.end()) {
			return dynamic_cast<T*>(item->second.get());
		}
		return nullptr;
	}
};

template<typename T>
class ComponentFactory {
public:
	template<typename... Args>
	static std::unique_ptr<T> Construct(Args&&... args)
	{ 
		return std::make_unique<T>(std::forward<Args>(args)...); 
	}
};
#endif
#ifndef OBJECT_H
#define OBJECT_H
#include <memory>
#include <unordered_map>
#include <typeindex>
#include "../rendering/model.hpp"
#include "component.hpp"
#include "transform.hpp"

class Object {
private:
	std::shared_ptr<Model> model;
	std::unordered_map<std::type_index, std::unique_ptr<Component>> components;
public:
	bool enabled;
	bool drawOverlay;
	Transform transform;

	Object(std::shared_ptr<Model>& model);

	template<typename T, typename... Args>
	T* AddComponent(Args&&... args) {
		auto item = components.find(std::type_index(typeid(T)));
		if (item == components.end()) {
			std::unique_ptr<T> obj = ComponentFactory<T>::Construct(components, transform, this, std::forward<Args>(args)...);
			T* ptr = obj.get();
			if (ptr) {
				auto pair = components.emplace(std::type_index(typeid(T)), std::move(obj));
				return ptr;
			}
			return nullptr;
		}
		return nullptr;
	}
	
	template<typename T>
	T* GetComponent() {
		auto item = components.find(std::type_index(typeid(T)));
		if (item != components.end()) {
			return static_cast<T*>(item->second.get());
		}
		return nullptr;
	}

	void Draw(Shader& shader) const;
	void Update();
	void FixedUpdate();
};
#endif
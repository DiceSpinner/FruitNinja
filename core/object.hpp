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
			auto ptr = std::make_unique<T>(components, transform, this, std::forward<Args>(args)...);
			auto handle = ptr.get();
			components.emplace(std::type_index(typeid(T)), std::move(ptr));
			return handle;
		}
		return nullptr;
	}
	
	template<typename T>
	T* GetComponent() {
		auto item = components.find(std::type_index(typeid(T)));
		if (item != components.end()) {
			return (T*)item->second.get();
		}
		return nullptr;
	}

	void Draw(Shader& shader) const;
	void Update();
	void FixedUpdate();
};
#endif
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
	bool alive;
public:
	Transform transform;

	Object(std::shared_ptr<Model>& model);

	template<typename T>
	void AddComponent() {
		auto item = components.find(std::type_index(typeid(T)));
		if (item == components.end()) {
			components.emplace(std::type_index(typeid(T)), std::make_unique<T>(components, transform));
		}
	}
	void Draw(Shader& shader) const;
	bool isAlive() const;
	virtual void Update();
	void Destroy();
};
#endif
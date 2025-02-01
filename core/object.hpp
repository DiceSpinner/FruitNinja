#ifndef OBJECT_H
#define OBJECT_H
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <typeindex>
#include "../rendering/model.hpp"
#include "component.hpp"
#include "transform.hpp"

class Object : public std::enable_shared_from_this<Object> {
private:
	/// <summary>
	/// The queue that stores all of the newly enabled objects, they will respond to updates in the next frame
	/// </summary>
	static std::unordered_set<std::shared_ptr<Object>> newObjectSet;
	/// <summary>
	/// A table of components, only 1 instance is allowed for each type
	/// </summary>
	std::unordered_map<std::type_index, std::unique_ptr<Component>> components;
	bool enabled;
	Object();
public:
	Transform transform;

	static std::shared_ptr<Object> Create();
	static void ActivateNewlyEnabledObjects();
	static void ExecuteFixedUpdate();
	static void ExecuteUpdate();

	template<typename T, typename... Args>
	T* AddComponent(Args&&... args) {
		auto item = components.find(std::type_index(typeid(T)));
		if (item == components.end()) {
			std::unique_ptr<T> obj = ComponentFactory<T>::Construct(components, transform, this, std::forward<Args>(args)...);
			if (obj) {
				if (enabled) {
					obj->OnEnabled();
				}
				auto ptr = obj.get();
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
			return dynamic_cast<T*>(item->second.get());
		}
		return nullptr;
	}

	void Update();
	void FixedUpdate();
	void SetEnable(bool value);
	bool IsActive() const;

	~Object();
};
#endif
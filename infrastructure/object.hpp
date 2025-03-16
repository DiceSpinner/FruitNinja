#ifndef OBJECT_H
#define OBJECT_H
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <typeindex>
#include "rendering/model.hpp"
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
	std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>> components;
	bool enabled;

	Object(bool isEnabled);
public:
	Transform transform;
	static void ActivateNewlyEnabledObjects();
	static void ExecuteEarlyFixedUpdate();
	static void ExecuteFixedUpdate();
	static void ExecuteUpdate();

	static std::shared_ptr<Object> Create(bool isEnabled = true);

	Object();

	template<typename T, typename... Args>
	T* AddComponent(Args&&... args) {
		std::unique_ptr<T> obj = ComponentFactory<T>::Construct(components, transform, this, std::forward<Args>(args)...);
		
		if (obj) {
			auto ptr = obj.get();
			auto item = components.find(std::type_index(typeid(T)));
			if (item == components.end()) {
				auto pair = components.emplace(std::type_index(typeid(T)), std::vector<std::unique_ptr<Component>>());
				pair.first->second.push_back(std::move(obj));
				if (enabled) {
					pair.first->second.back()->OnEnabled();
				}
			}
			else {
				item->second.push_back(std::move(obj));
				if (enabled) {
					item->second.back()->OnEnabled();
				}
			}
			return ptr;
		}
		return nullptr;
	}
	
	template<typename T>
	T* GetComponent(size_t index = 0) {
		auto item = components.find(std::type_index(typeid(T)));
		if (item != components.end()) {
			return dynamic_cast<T*>(item->second.at(index).get());
		}
		return nullptr;
	}

	void SetEnable(bool value);
	bool IsActive() const;
};
#endif
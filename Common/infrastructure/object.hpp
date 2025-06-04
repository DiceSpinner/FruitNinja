#ifndef OBJECT_H
#define OBJECT_H
#include <memory>
#include <unordered_map>
#include <list>
#include <typeindex>
#include "rendering/model.hpp"
#include "clock.hpp"
#include "transform.hpp"

class ObjectManager;
class Component;

template<typename T>
class ComponentFactory;

class Object : public std::enable_shared_from_this<Object> {
	friend class ObjectManager;
private:
	bool isUpdating = false;
	bool signaledDetachment = false;
	// The manager that currently manages this object, only 1 manager can be manager this object at the same time
	ObjectManager* manager = nullptr; 
	// Iterator to ObjectManager::updateList for fast removal
	std::list<Object*>::iterator pointer = {};
	std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>> components = {};

	// Only called by ObjectManager
	void DisableAllComponents() const;
	void EnableAllComponents() const;
public:
	Transform transform = {};

	Object();
	Object(const Object&) = delete;
	Object(Object&&) = delete;
	Object& operator = (const Object&) = delete;
	Object& operator = (Object&&) = delete;
	~Object();

	template<typename T, typename... Args>
	T* AddComponent(Args&&... args) {
		std::unique_ptr<T> obj = ComponentFactory<T>::Construct(components, transform, this, std::forward<Args>(args)...);
		
		if (obj) {
			auto ptr = obj.get();
			auto item = components.find(std::type_index(typeid(T)));
			if (item == components.end()) {
				auto pair = components.emplace(std::type_index(typeid(T)), std::vector<std::unique_ptr<Component>>());
				pair.first->second.push_back(std::move(obj));
				if (manager) {
					pair.first->second.back()->OnEnabled();
				}
			}
			else {
				item->second.push_back(std::move(obj));
				if (manager) {
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

	void Detach();
	bool IsActive() const;
	ObjectManager* Manager() const;
};

class ObjectManager {
	friend class Object;
private:
	std::unordered_map<Object*, std::shared_ptr<Object>> activeObjects;
	/// <summary>
	/// The queue that stores all of the newly enabled objects, they will respond to updates in the next frame
	/// </summary>
	std::list<Object*> updateList = {};
	void ExecuteEarlyFixedUpdate(Clock& clock);
	void ExecuteFixedUpdate(Clock& clock);
	void ExecuteUpdate(Clock& clock);
public:
	ObjectManager();
	ObjectManager(const ObjectManager& other) = delete;
	ObjectManager(ObjectManager&& other) = delete;
	ObjectManager& operator = (const ObjectManager&) = delete;
	ObjectManager& operator = (ObjectManager&&) = delete;
	~ObjectManager();

	std::shared_ptr<Object> CreateObject();
	void Register(const std::shared_ptr<Object>& obj);
	void Unregister(Object* obj);
	void Tick(Clock& clock);
};



class Component {
	template <typename T> friend class ComponentFactory;
private:
	std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& componentMap;
public:
	Object* const object;
	Transform& transform;
	Component(std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& collection, Transform& transform, Object* object);
	void virtual Update(Clock& clock);
	void virtual Initialize();
	void virtual EarlyFixedUpdate(Clock& clock);
	void virtual FixedUpdate(Clock& clock);
	void virtual OnEnabled();
	void virtual OnDisabled();
	virtual ~Component() = default;

	template<typename T>
	T* GetComponent(size_t index = 0) {
		auto item = componentMap.find(std::type_index(typeid(T)));
		if (item != componentMap.end()) {
			return dynamic_cast<T*>(item->second.at(index).get());
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
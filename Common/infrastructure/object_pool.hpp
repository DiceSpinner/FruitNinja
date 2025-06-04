#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H
#include <stack>
#include <memory>
#include <functional>

template<typename T>
class ObjectPool : public std::enable_shared_from_this<ObjectPool<T>>{
private:
	size_t size;
	std::stack<T*> pool;
public:
	template<typename... Args>
	ObjectPool(size_t poolSize, std::function<T*()> constructor) : size(poolSize) {
		for (auto i = 0; i < poolSize;i++) {
			pool.emplace(constructor());
		}
	}

	std::unique_ptr<T, std::function<void(T*)>> Acquire() {
		if (!pool.empty()) {
			auto item = pool.top();
			pool.pop();
			std::weak_ptr<ObjectPool<T>> weakPtr = this->shared_from_this();
			auto deleter = [weakPtr](T* ptr) { 
				auto poolPtr = weakPtr.lock();
				if (poolPtr) {
					poolPtr->pool.push(ptr);
				}
				else {
					delete ptr;
				}
			};
			return std::unique_ptr < T, decltype(deleter) > (item, deleter);
		}
		return {};
	}

	ObjectPool(const ObjectPool<T>& other) = delete;
	ObjectPool(ObjectPool<T>&& other) = delete;
	ObjectPool& operator =(const ObjectPool<T> other) = delete;
	ObjectPool& operator =(ObjectPool<T>&& other) = delete;

	~ObjectPool() {
		while (!pool.empty()) {
			delete pool.top();
			pool.pop();
		}
	}
};

#endif
#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H
#include <stack>
#include <memory>
#include <functional>

template<typename T>
class ObjectPool {
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
			auto deleter = [this](T* ptr) { 
				pool.push(ptr); 
			};
			return std::unique_ptr < T, decltype(deleter) > (item, deleter);
		}
		return {};
	}

	~ObjectPool() {
		while (!pool.empty()) {
			delete pool.top();
			pool.pop();
		}
	}
};

#endif
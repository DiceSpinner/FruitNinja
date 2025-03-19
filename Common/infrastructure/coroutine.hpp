#ifndef COROUTINE_H
#define COROUTINE_H
#include <memory>
#include <list>
#include <coroutine>
#include <optional>

struct Coroutine {
public:
	struct YieldOption {
		friend struct Coroutine;
	public:
		static YieldOption Wait(float timeToWait);
		static YieldOption WaitUnscaled(float timeToWait);

		float waitCounter = 0;
		float waitTime = 0;
		bool scaled = true;
	};
	struct promise_type {
		std::optional<YieldOption> option;
		Coroutine get_return_object() noexcept;
		std::suspend_always initial_suspend() noexcept;
		std::suspend_always final_suspend() noexcept;
		std::suspend_always yield_value(std::optional<YieldOption> value) noexcept;
		void return_void() noexcept;
		void unhandled_exception() noexcept;
	};
private:
	std::coroutine_handle<promise_type> handle;
public:
	Coroutine(promise_type* p) noexcept;
	Coroutine(Coroutine&& other) noexcept;
	~Coroutine();

	bool Advance() const;
	bool Finished() const;
};

class CoroutineManager {
	std::list<std::shared_ptr<Coroutine>> coroutines;
public:
	CoroutineManager() = default;
	void Run();
	std::weak_ptr<Coroutine> AddCoroutine(Coroutine&& coroutine);
	bool Empty() const;
};

#endif
#include "coroutine.hpp"
#include "state/time.hpp"
#include <iostream>
using namespace std;

Coroutine::Coroutine(promise_type* p) noexcept : handle(coroutine_handle<promise_type>::from_promise(*p)) {}
Coroutine::Coroutine(Coroutine&& other) noexcept : handle(exchange(other.handle, nullptr)) {}
Coroutine::~Coroutine() {
	if (handle) {
		handle.destroy();
	}
}
bool Coroutine::Advance() const {
	if (handle.done()) {
		return false;
	}
	auto& yieldOption = handle.promise().option;
	if (yieldOption) {
		auto& option = yieldOption.value();

		// Only continue if the specified amount of time has passed
		if (option.waitTime > 0) {
			if (option.scaled) {
				option.waitCounter += Time::deltaTime();
			}
			else {
				option.waitCounter += Time::unscaledDeltaTime();
			}
			if (option.waitCounter < option.waitTime) {
				return true;
			}
		}
	}

	handle.resume();

	return !handle.done();
}

bool Coroutine::Finished() const {
	return handle.done();
}

#pragma region YieldOption
Coroutine::YieldOption Coroutine::YieldOption::Wait(float timeToWait) {
	return Coroutine::YieldOption { .waitTime = timeToWait };
}

Coroutine::YieldOption Coroutine::YieldOption::WaitUnscaled(float timeToWait) {
	return Coroutine::YieldOption{ .waitTime = timeToWait, .scaled = false };
}

#pragma endregion

#pragma region PromiseType
Coroutine Coroutine::promise_type::get_return_object() noexcept { return { this }; }
std::suspend_always Coroutine::promise_type::initial_suspend() noexcept { return {}; }
std::suspend_always Coroutine::promise_type::final_suspend() noexcept { return {}; }
std::suspend_always Coroutine::promise_type::yield_value(std::optional<YieldOption> value) noexcept { option = value; return {}; }
void Coroutine::promise_type::return_void() noexcept { option = {}; }
void Coroutine::promise_type::unhandled_exception() noexcept { std::terminate(); }
#pragma endregion

void CoroutineManager::Run() {
	for (auto i = coroutines.begin(); i != coroutines.end();) {
		auto& coroutine = *i;
		if (coroutine->Finished() || !coroutine->Advance()) {
			i = coroutines.erase(i);
			continue;
		}
		++i;
	}
}

weak_ptr<Coroutine> CoroutineManager::AddCoroutine(Coroutine&& task) {
	return coroutines.emplace_back(make_shared<Coroutine>(std::forward<Coroutine>(task)));
}

bool CoroutineManager::Empty() const {
	return coroutines.empty();
}
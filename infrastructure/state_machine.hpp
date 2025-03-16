#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H
#include <optional>
#include <memory>
#include <typeindex>
#include <unordered_map>

class State {
private:
	std::unordered_map<std::type_index, std::unique_ptr<State>> states = {};
	State* currSubState = nullptr;
public:
	State() = default;
	~State() = default;
	State(const State& other) = delete;
	State(State&& other) = delete;
	State& operator = (const State& other) = delete;
	State& operator = (State&& other) = delete;

	template<typename StateType, typename... Args>
	State* AddSubState(Args&&... args) {
		auto type = std::type_index(typeid(StateType));
		auto state = states.find(type);
		if (state != states.end()) {
			return;
		}
		auto& inserted = states.emplace(type, std::make_unique<StateType>(std::forward<Args>(args)...)).first->second;
		inserted->Init();
	}
	
	template<typename StateType>
	bool SetCurrSubState() {
		auto type = std::type_index(typeid(StateType));
		auto state = states.find(type);
		if (state == states.end()) {
			return false;
		}
		if (state->second.get() == currSubState) {
			return true;
		}
		if(currSubState) currSubState->OnExit();
		currSubState = state->second.get();
		currSubState->OnEnter();
		return true;
	}

	std::optional<std::type_index> Current();
	std::optional<std::type_index> Run();
	virtual void Init() = 0;
	virtual void OnEnter() = 0;
	virtual void OnExit() = 0;
	virtual std::optional<std::type_index> Step() = 0;
};

#endif
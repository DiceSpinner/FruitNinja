#include <iostream>
#include "state_machine.hpp"

using namespace std;

std::optional<std::type_index> State::Run() {
	if (!currSubState) { return Step(); };

	optional<type_index> next = currSubState->Step();
	if (next && next.value() != type_index(typeid(currSubState))) {
		auto nextState = states.find(next.value());
		if (nextState == states.end()) {
			cout << "Attemping to transition into non-existing state!\n";
			currSubState->OnExit();
			return {};
		}
		currSubState->OnExit();
		currSubState = nextState->second.get();
		currSubState->OnEnter();
	}
	return type_index(typeid(this));
}

optional<type_index> State::Current() {
	if (!currSubState) return {};
	return type_index(typeid(currSubState));
}
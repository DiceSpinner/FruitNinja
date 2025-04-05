#include "default_state.hpp"
#include "connection_state.hpp"

using namespace std;

DefaultState::DefaultState(Server& server) : ServerState(server) {}

void DefaultState::Init(){
	AddSubState<ConnectionState>();
}

void DefaultState::OnEnter() {
	EnterSubState<ConnectionState>();
}

optional<type_index> DefaultState::Step() {
	return {};
}
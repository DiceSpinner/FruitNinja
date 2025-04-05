#include "connection_state.hpp"

using namespace std;

ConnectionState::ConnectionState(Server& server) : ServerState(server) {}

void ConnectionState::Init(){}

void ConnectionState::OnEnter() {

}

void ConnectionState::OnExit() {}

optional<type_index> ConnectionState::Step() {
	return {};
}
#include "server.hpp"

using namespace std;
ServerState::ServerState(Server& server) : server(server) {}

void ServerState::Terminate() { terminated = true; OnExit(); }
void ServerState::OnEnterSubState() {}
void ServerState::OnExitSubState() {}
void ServerState::OnEnter() {}
void ServerState::OnExit() {}
void ServerState::ProcessInput() {}
void ServerState::Init() {}
type_index ServerState::Self() const { return type_index(typeid(*this)); }
void ServerState::StartCoroutine(Coroutine&& coroutine) {
	coroutineManager.AddCoroutine(forward<Coroutine>(coroutine));
}
optional<type_index> ServerState::Step() { return {}; };
optional<type_index> ServerState::Run() {
	// Coroutines must be finished before states can be runned
	if (!coroutineManager.Empty()) {
		coroutineManager.Run();
		return Self();
	}
	if (terminated) { terminated = false; return transition; }

	if (enterSubState) {
		currSubState = std::exchange(enterSubState, {});
		currSubState->OnEnter();
	}

	// Run current state if there's no substate
	if (!currSubState) {
		auto next = Step();
		if (!next || next.value() != Self()) {
			transition = next;
			Terminate(); return Self();
		}
		return next;
	}

	// Run sub state
	auto next = currSubState->Run();
	if (!next) {
		currSubState = {};
		OnExitSubState();
	}
	else if (next.value() != type_index(typeid(*currSubState))) {
		auto result = states.find(next.value());
		if (result != states.end()) {
			currSubState = result->second.get();
			currSubState->OnEnter();
		}
		else {
			cout << "State not found" << endl;
		}
	}
	return Self();
}
void ServerState::BroadCastState() {}

Server::Server() : running(true) {
	network.player1 = std::make_unique<UDPConnection>();
	network.player2 = std::make_unique<UDPConnection>();
	if (!network.player1->VerifyConnection() || !network.player2->VerifyConnection()) {
		std::cout << "Not enough sockets available, shutting down server.\n";
		running = false;
	}
}
void Server::CleanUp() { network.player1->Close(); network.player2->Close(); }
void Server::ProcessInput() { if(state) state->ProcessInput(); }
void Server::Step() {if(state) state->Run(); }
void Server::BroadCastState() { if (state) state->BroadCastState(); }
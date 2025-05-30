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

Server::Server(TimeoutSetting timeout) : running(true), timeout(timeout), startTime(std::chrono::steady_clock::now())
{
	context.connectionManager = std::make_unique<UDPConnectionManager>(30000, 10, 1500, std::chrono::milliseconds(10));
	context.connectionManager->isListening = true;
	if (!context.connectionManager->Good()) {
		std::cout << "Connection managed failed to initialzie" << "\n";
		return;
	}
	
}

void Server::CleanUp() { 
	context.player1->Disconnect(); 
	context.player2->Disconnect();
	context.connectionManager->isListening = false;
}
void Server::ProcessInput() { 
	bool disconnect = false;
	if (!context.player1) {
		context.player1 = context.connectionManager->Accept(timeout);
	}
	else if (context.player1->Closed()) {
		
	}
}

void Server::Step() { 
	auto time = std::chrono::steady_clock::now() - startTime;
	std::cout << "[Time] " << std::chrono::duration_cast<std::chrono::milliseconds>(time) << std::endl; 
}
void Server::BroadCastState() { ; }
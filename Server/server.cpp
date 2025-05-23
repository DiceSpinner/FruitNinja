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

Server::Server() : running(true), startTime(std::chrono::steady_clock::now())
{
	context.connectionManager = std::make_unique<UDPConnectionManager<2>>(30000, 10, 1500, std::chrono::milliseconds(10));
	context.connectionManager->isListening = true;
	if (!context.connectionManager->Good()) {
		std::cout << "Connection managed failed to initialzie" << "\n";
		return;
	}
	networkMonitorThread = std::move(std::thread(&Server::MonitorConnection, this));
}

void Server::MonitorConnection() {
	TimeoutSetting timeout{
		.connectionTimeout = std::chrono::milliseconds(5000),
		.connectionRetryInterval = std::chrono::milliseconds(300),
		.requestTimeout = std::chrono::milliseconds(3000),
		.requestRetryInterval = std::chrono::milliseconds(300),
		.impRetryInterval = std::chrono::milliseconds(300)
	};
	while (running) {
		if (!context.player1) {
			auto player1 = context.connectionManager->Accept(timeout);
			if (player1.expired()) continue;
			std::lock_guard<std::mutex> guard(context.lock);
			context.player1 = player1.lock();
		}
		else if (context.player1->Closed()) {
			std::lock_guard<std::mutex> guard(context.lock);
			context.player1.reset();
		}
		if (!context.player2) {
			auto player2 = context.connectionManager->Accept(timeout);
			if (player2.expired()) continue;
			std::lock_guard<std::mutex> guard(context.lock);
			context.player2 = player2.lock();
		}
		else if (context.player2->Closed()) {
			std::lock_guard<std::mutex> guard(context.lock);
			context.player2.reset();
		}
	}
}

void Server::CleanUp() { 
	context.player1->Disconnect(); 
	context.player2->Disconnect();
	context.connectionManager->isListening = false;
}
void Server::ProcessInput() {  }
void Server::Step() { 
	auto time = std::chrono::steady_clock::now() - startTime;
	std::cout << "[Time] " << std::chrono::duration_cast<std::chrono::milliseconds>(time) << std::endl; 
}
void Server::BroadCastState() { ; }
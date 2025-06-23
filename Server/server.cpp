#include "server.hpp"

Server::Server(TimeoutSetting timeout, uint32_t fps) : running(true), timeout(timeout), gameClock(fps)
{
	connectionManager = std::make_unique<UDPConnectionManager>(30000, 10, 1500, std::chrono::milliseconds(10));
	connectionManager->isListening = true;
	if (!connectionManager->Good()) {
		std::cout << "Connection managed failed to initialzie" << "\n";
		return;
	}
	
}

void Server::Terminate() { 
	player1->Disconnect(); 
	player2->Disconnect();
	connectionManager->isListening = false;
}

void Server::ChangeState(ServerState newState) {
	if (state == newState) return;
	OnExitState();
	state = newState;
	OnEnterState();
}

void Server::OnEnterState() {
	if (state == ServerState::OnHold) { // When player disconnects
		// Send message to tell the connected client the other player has disconnected
		if (!player1->IsConnected() && player2->IsConnected()) {
		}
		else if (player1->IsConnected() && !player2->IsConnected()) {

		}
	}
	else if (state == ServerState::Score) {
		// Send message to both
	}
}

void Server::OnExitState() {

}

void Server::UpdateState() {
	if (state == ServerState::OnHold) {
		if (!player1->IsConnected()) return;
		if (!player2->IsConnected()) return;
		ChangeState(Server::InGame);
	}
	else if (state == ServerState::InGame) {
		if (!player1->IsConnected() || !player2->IsConnected()) ChangeState(ServerState::OnHold);
		game.Step(gameClock, player1, player2);
	}
	else if(state == ServerState::Score ){
		if (!player1->IsConnected() || !player2->IsConnected()) ChangeState(ServerState::OnHold);
	}
}


void Server::ProcessInput() {
	gameClock.Tick();
	if (state == ServerState::OnHold) {
		if (!player1->IsConnected()) player1 = connectionManager->Accept(timeout);
		if (!player2->IsConnected()) player2 = connectionManager->Accept(timeout);
		return;
	}

	if (state == ServerState::InGame) {
		game.ProcessInput(player1, player2);
		return;
	}
}

void Server::Step() { 
	if (state != ServerState::OnHold) {
		game.Step(gameClock, player1, player2);
	}
}
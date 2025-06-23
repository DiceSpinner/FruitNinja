#include "multiplayer_game.hpp"

Game::Game() {

}

void Game::Step(const Clock& clock, std::shared_ptr<UDPConnection>& player1, std::shared_ptr<UDPConnection>& playe2) {
	
}

void Game::ProcessInput(std::shared_ptr<UDPConnection>& player1, std::shared_ptr<UDPConnection>& player2) {
	
}

void Game::Reset() {
	context1 = {};
	context2 = {};
	objManager.UnregisterAll();
}
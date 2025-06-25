#include "multiplayer_game.hpp"

Game::Game() {

}

void Game::Step(const Clock& clock, std::shared_ptr<LiteConnConnection>& player1, std::shared_ptr<LiteConnConnection>& playe2) {
	
}

void Game::ProcessInput(std::shared_ptr<LiteConnConnection>& player1, std::shared_ptr<LiteConnConnection>& player2) {
	
}

void Game::Reset() {
	context1 = {};
	context2 = {};
	objManager.UnregisterAll();
}
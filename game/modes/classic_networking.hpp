#ifndef CLASSIC_NETWORK_H
#define CLASSIC_NETWORK_H
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include "game/game.hpp"
#include "game/fruit.hpp"
#include "game/bomb.hpp"
#include "infrastructure/ui.hpp"
#include "game/vfx.hpp"

class NClassicMode : public GameState {
private:
	class Selection : public GameState {
	private:
		NClassicMode* mode;
		struct {
			std::unique_ptr<UI> guestText;
			std::unique_ptr<UI> hostText;
			std::unique_ptr<UI> backText;
			std::shared_ptr<Object> guestSelection;
			std::shared_ptr<Object> hostSelection;
			std::shared_ptr<Object> backSelection;
		} ui;
	public:
		Selection(Game* game, NClassicMode* mode);
		void Init() override;
		void OnEnter() override;
		void OnExit() override;
		void OnDrawFrontUI(Shader& uiShader) override;
		bool AllowParentBackUI() override;
		std::optional < std::type_index > Step() override;
	};


	struct {
		SOCKET client;
		SOCKET listener;
		SOCKET server;
	} network;
	struct {
		FruitChannel fruitChannel;
		BombChannel bombChannel;

		std::unique_ptr<State> state;

		float explosionTimer = 0;
		float spawnTimer = 0;
		float spawnCooldown = 1;
		bool drawFadeOut = false;
	} context;
	struct {
		// Fruit UI
		std::shared_ptr<Object> startGame;
		std::shared_ptr<Object> exitGame;
		std::shared_ptr<Object> restart;

		// Back UI
		std::unique_ptr<UI> background;
		std::unique_ptr<UI> backgroundText;

		// Front UI
		std::unique_ptr<UI> sinfrastructureBoard;
		std::unique_ptr<UI> largeSinfrastructureBoard;
		std::unique_ptr<UI> startButton;
		std::unique_ptr<UI> exitButton;
		std::unique_ptr<UI> restartButton;
		std::unique_ptr<UI> redCrosses[3];
		std::unique_ptr<UI> emptyCrosses[3];
		std::unique_ptr<UI> fadeOutEffect;
	} ui;
	struct {
		float sizeWatermelon = 1.5f;
		float sizePineapple = 1.0f;
		float sizeApple = 1.0f;
		float sizeBomb = 0.8f;
		float fruitSpawnCenter = 0;
		float fruitSpawnWidth = 15;
		float fruitSpawnHeight = -13;
		float fruitKillHeight = -15;
		float bombKillHeight = -40;
		float fruitUpMax = 32;
		float fruitUpMin = 25;
		float fruitHorizontalMin = -7;
		float fruitHorizontalMax = 7;
		float fruitSliceForce = 10;

		float spawnAmountMin = 1;
		float spawnAmountMax = 5;
		float spawnCooldownMin = 0.5;
		float spawnCooldownMax = 3;
		glm::vec3 spawnMinRotation = { 30, 30, 30 };
		glm::vec3 spawnMaxRotation = { 90, 90, 90 };
		unsigned int missTolerence = 3;

		// UI
		glm::vec2 startButtonPos = { -0.5, -0.5 };
		glm::vec2 exitButtonPos = { 0.5, -0.5 };
		glm::vec3 finalSinfrastructurePos = { 0, 0, 0 };

		float explosionDuration = 2;
		glm::vec4 sinfrastructureColor = { 1, 1, 0, 1 };
		glm::vec4 startColor = { 0, 1, 0, 1 };
		glm::vec4 exitColor = { 1, 0, 0, 1 };
		glm::vec4 restartColor = { 1, 1, 0, 1 };
		float fadeStart = 1;
	} setting;
	
	std::vector<std::function<void()>> fruitSpawners;
public:
	NClassicMode(Game* game);
	void Init() override;
	void OnEnter();
	void OnExit();
	std::optional<std::type_index> Step() override;
	void OnDrawVFX(Shader& vfxShader) override;
	void OnDrawBackUI(Shader& uiShader) override;
	void OnDrawFrontUI(Shader& uiShader) override;

	std::shared_ptr<Object> createUIObject(std::shared_ptr<Model> model) const;
	std::shared_ptr<Object> createUIObject(std::shared_ptr<Model> model, glm::vec4 outlineColor) const;
	void spawnApple();
	void spawnPineapple();
	void spawnWatermelon();
	void spawnBomb();
	void spawnFruit(
		std::shared_ptr<Model>& fruitModel,
		std::shared_ptr<Model>& slice1Model,
		std::shared_ptr<Model>& slice2Model,
		std::shared_ptr<AudioClip> sliceAudio,
		glm::vec4 color, float radius = 1, int sinfrastructure = 1
	);
};

#endif
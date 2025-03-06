#ifndef CLASSIC_H
#define CLASSIC_H
#include <functional>
#include <vector>
#include "game.hpp"
#include "vfx.hpp"
#include "../core/ui.hpp"
#include "../core/object_pool.hpp"
#include "fruit.hpp"
#include "bomb.hpp"

struct ClassicModeUI {
	// Fruit UI
	std::shared_ptr<Object> startGame;
	std::shared_ptr<Object> exitGame;
	std::shared_ptr<Object> restart;

	// Back UI
	std::unique_ptr<UI> background;
	std::unique_ptr<UI> backgroundText;

	// Front UI
	std::unique_ptr<UI> scoreBoard;
	std::unique_ptr<UI> largeScoreBoard;
	std::unique_ptr<UI> startButton;
	std::unique_ptr<UI> exitButton;
	std::unique_ptr<UI> restartButton;
	std::unique_ptr<UI> redCrosses[3];
	std::unique_ptr<UI> emptyCrosses[3];
	std::unique_ptr<UI> fadeOutEffect;
};

struct ClassicModeSetting {
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
	glm::vec3 finalScorePos = { 0, 0, 0 };
	
	float explosionDuration = 2;
	glm::vec4 scoreColor = { 1, 1, 0, 1 };
	glm::vec4 startColor = { 0, 1, 0, 1 };
	glm::vec4 exitColor = { 1, 0, 0, 1 };
	glm::vec4 restartColor = { 1, 1, 0, 1 };
	float fadeStart = 1;
};

struct ClassicModeState {
	enum State {
		START,
		GAME,
		EXPLOSION,
		SCORE
	} current;

	FruitChannel fruitChannel;
	BombChannel bombChannel;

	float explosionTimer = 0;

	float spawnTimer = 0;
	float spawnCooldown = 1;

	bool drawFadeOut = false;
};

class ClassicMode : public GameMode {
private:
	ClassicModeState state;
	ClassicModeUI ui;
	ClassicModeSetting setting;
	ExplosionVFX vfx;
	std::vector<std::function<void()>> fruitSpawners;

	std::shared_ptr<Object> createUIObject(std::shared_ptr<Model> model) const;
	std::shared_ptr<Object> createUIObject(std::shared_ptr<Model> model, glm::vec4 outlineColor) const;
	void enterGame();
	void enterExplosion();
	void enterScore();
	void exitExplosion();
	void exitScore();
	void processStart();
	void processGame();
	void processExplosion();
	void processScore();
	
	void spawnApple();
	void spawnPineapple();
	void spawnWatermelon();
	void spawnBomb();
	void spawnFruit(
		std::shared_ptr<Model>& fruitModel, 
		std::shared_ptr<Model>& slice1Model, 
		std::shared_ptr<Model>& slice2Model, 
		std::shared_ptr<AudioClip> sliceAudio, 
		glm::vec4 color, float radius = 1, int score = 1
	);

public:
	ClassicMode(Game* game);
	void Init() override;
	void OnEnter();
	void OnExit();
	std::optional<std::type_index> Step() override;
	void DrawVFX(Shader& vfxShader) override;
	void DrawBackUI(Shader& uiShader) override;
	void DrawFrontUI(Shader& uiShader) override;
};

#endif
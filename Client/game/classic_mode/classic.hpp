#ifndef CLASSIC_H
#define CLASSIC_H
#include <functional>
#include <vector>
#include "game/game.hpp"
#include "game/vfx.hpp"
#include "infrastructure/state_machine.hpp"
#include "infrastructure/ui.hpp"
#include "infrastructure/object_pool.hpp"
#include "game/bomb.hpp"

struct ClassicModeUI {
	// Fruit UI
	std::shared_ptr<Object> startGame;
	std::shared_ptr<Object> back;
	std::shared_ptr<Object> restart;

	// Back UI
	std::unique_ptr<UI> background;
	std::unique_ptr<UI> backgroundText;

	// Front UI
	std::unique_ptr<UI> scoreBoard;
	std::unique_ptr<UI> largeScoreBoard;
	std::unique_ptr<UI> startButton;
	std::unique_ptr<UI> backButton;
	std::unique_ptr<UI> restartButton;
	std::unique_ptr<UI> redCrosses[3];
	std::unique_ptr<UI> emptyCrosses[3];
	std::unique_ptr<UI> fadeOutEffect;
};

struct ClassicModeSetting {
	float sizeWatermelon = 1.5f;
	float sizePineapple = 1.0f;
	float sizeApple = 1.0f;
	float sizeCoconut = 1.2f;
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

struct ClassicModeContext {
	enum {
		Start,
		Game,
		Explosion,
		Score
	} current;

	FruitChannel fruitChannel;
	BombChannel bombChannel;

	float explosionTimer = 0;

	float spawnTimer = 0;
	float spawnCooldown = 1;

	bool drawFadeOut = false;
};

class ClassicMode : public GameState {
private:
	ClassicModeContext context;
	ClassicModeUI ui;
	ClassicModeSetting setting;
	ExplosionVFX vfx;
	std::vector<std::function<void()>> fruitSpawners;

	void enterGame();
	void enterExplosion();
	void enterScore();
	void exitExplosion();
	void exitScore();
	void processStart();
	void processGame();
	void processExplosion();
	
	void spawnApple();
	void spawnPineapple();
	void spawnWatermelon();
	void spawnCoconut();
	void spawnBomb();
	void spawnFruit(
		std::shared_ptr<Model>& fruitModel, 
		std::shared_ptr<Model>& slice1Model, 
		std::shared_ptr<Model>& slice2Model, 
		std::shared_ptr<AudioClip> sliceAudio, 
		glm::vec4 color, float radius = 1, int sinfrastructure = 1
	);
	void PositionUI();
	Coroutine FadeInUI(float duration);
	Coroutine FadeOutUI(float duration);
public:
	ClassicMode(Game& game);
	void Init() override;
	void OnEnter() override;
	void OnExit() override;
	std::optional<std::type_index> Step() override;
	void OnDrawVFX(Shader& vfxShader) override;
	void OnDrawBackUI(Shader& uiShader) override;
	void OnDrawFrontUI(Shader& uiShader) override;
	bool AllowParentFrontUI() override;
};

#endif
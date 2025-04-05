#ifndef GAME_H
#define GAME_H
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <unordered_map>
#include <list>
#include <typeindex>
#include <iostream>
#include "state/window.hpp"
#include "infrastructure/object.hpp"
#include "infrastructure/state_machine.hpp"
#include "infrastructure/coroutine.hpp"
#include "rendering/model.hpp"
#include "audio/audio_clip.hpp"
#include "game/fruit.hpp"

struct GameModels {
	std::shared_ptr<Model> appleModel;
	std::shared_ptr<Model> appleTopModel;
	std::shared_ptr<Model> appleBottomModel;
	std::shared_ptr<Model> pineappleModel;
	std::shared_ptr<Model> pineappleTopModel;
	std::shared_ptr<Model> pineappleBottomModel;
	std::shared_ptr<Model> watermelonModel;
	std::shared_ptr<Model> watermelonTopModel;
	std::shared_ptr<Model> watermelonBottomModel;
	std::shared_ptr<Model> coconutModel;
	std::shared_ptr<Model> coconutTopModel;
	std::shared_ptr<Model> coconutBottomModel;
	std::shared_ptr<Model> bombModel;
};

struct GameAudios {
	std::shared_ptr<AudioClip> fruitSliceAudio1;
	std::shared_ptr<AudioClip> fruitSliceAudio2;
	std::shared_ptr<AudioClip> startMenuAudio;
	std::shared_ptr<AudioClip> gameStartAudio;
	std::shared_ptr<AudioClip> gameOverAudio;
	std::shared_ptr<AudioClip> fruitMissAudio;
	std::shared_ptr<AudioClip> fruitSpawnAudio;
	std::shared_ptr<AudioClip> recoveryAudio;
	std::shared_ptr<AudioClip> explosionAudio;
	std::shared_ptr<AudioClip> fuseAudio;
};

struct GameTextures {
	GLuint sliceParticleTexture;
	GLuint sparkTexture;
	GLuint smokeTexture;
	GLuint backgroundTexture;
	GLuint fadeTexture;
	GLuint emptyCross;
	GLuint redCross;
};

class GameState;
class Game;

template<typename T>
concept TGameState = std::derived_from<T, GameState>;

class GameState {
private:
	std::unordered_map<std::type_index, std::unique_ptr<GameState>> states = {};
	CoroutineManager coroutineManager = {};
	GameState* currSubState = nullptr;
	std::optional<std::type_index> transition;
	bool terminated;
	GameState* enterSubState;

	void Terminate();
protected:
	Game* game;

	template<TGameState T>
	std::optional<std::type_index> EnterSubState() {
		if (terminated || currSubState) {
			std::cout << "Cannot enter sub state while terminated/has active sub state" << std::endl;
			return {};
		}
		auto type = std::type_index(typeid(T));
		auto state = states.find(type);
		if (state == states.end()) {
			std::cout << "Cannot find substate" << std::endl;
			return {};
		}
		enterSubState = state->second.get();
		OnEnterSubState();
		return Self();
	}
	
	virtual void OnEnterSubState();
	virtual void OnExitSubState();
	virtual void OnDrawVFX(Shader& vfxShader);
	virtual void OnDrawBackUI(Shader& uiShader);
	virtual void OnDrawFrontUI(Shader& uiShader);
	virtual std::optional<std::type_index> Step();
	void StartCoroutine(Coroutine&& coroutine);
	inline std::type_index Self() const;

	template<TGameState T>
	inline std::type_index Transition() const {
		return std::type_index(typeid(T));
	}
public:
	GameState(Game* game);

	template<TGameState T, typename... Args>
	T* AddSubState(Args&&... args) {
		auto type = std::type_index(typeid(T));
		auto state = states.find(type);
		if (state != states.end()) {
			return {};
		}
		auto& inserted = states.emplace(type, std::make_unique<T>(std::forward<Args>(args)...)).first->second;
		inserted->Init();

		return static_cast<T*>(inserted.get());
	}

	void DrawVFX(Shader& vfxShader);
	void DrawBackUI(Shader& uiShader);
	void DrawFrontUI(Shader& uiShader);
	std::optional<std::type_index> Run();

	virtual void Init() = 0;
	virtual void OnEnter() = 0;
	virtual void OnExit() = 0;
	
	virtual bool AllowParentBackUI();
	virtual bool AllowParentFrontUI();
};

struct MouseTrailSetting {
	size_t maxMousePosSize = 100;
	size_t maxTrailSize = 20;
	float arrowSize = 50;
	float mousePosKeepTime = 0.3f;
};

class Game {
private:
	std::unique_ptr<GameState> state;
	MouseTrailSetting mouseTrailSetting;
	bool flushMousePositions = false;
	std::unique_ptr<Shader> trailShader;
	GLuint trailTexture;
	GLuint trailArrow;
	std::list<glm::vec3> mousePositions;
	GLuint mouseTrailVAO;
	GLuint mouseTrailVBO;
	GLuint mouseTrailEBO;

	void DrawMouseTrailing();
public:
	struct {
		float sizeWatermelon = 1.5f;
		float sizePineapple = 1.0f;
		float sizeApple = 1.0f;
		float sizeCoconut = 1.2f;
		float sizeBomb = 0.8f;
		float fruitSpawnCenter = 0;
		float fruitKillHeight = -15;
		float fruitSliceForce = 10;

		FruitChannel control;
		glm::vec3 spawnMinRotation = { 30, 30, 30 };
		glm::vec3 spawnMaxRotation = { 90, 90, 90 };
	} uiConfig;

	std::shared_ptr<Object> player;
	GameModels models;
	GameAudios audios;
	GameTextures textures;

	// Mouse Trail
	bool enableMouseTrail = true;

	Game();
	~Game();
	void Init();

	static Object* createFruitParticleSystem();

	template<TGameState T>
	void SetInitialGameState() {
		if (state) return;
		auto type = std::type_index(typeid(T));
		if (std::type_index(typeid(state.get())) == type) return;

		state = std::make_unique<T>(this);
		state->Init();
		state->OnEnter();
	}

	void OnMouseUpdate(glm::vec2 position);
	void OnLeftReleased();
	void LoadAudios();
	void LoadModels();
	void LoadTextures();
	void DrawVFX(Shader& vfxShader);
	void DrawFrontUI(Shader& uiShader);
	void DrawBackUI(Shader& uiShader);
	void Step();
	std::shared_ptr<Object> createUIObject(std::shared_ptr<Model> model) const;
	std::shared_ptr<Object> createUIObject(std::shared_ptr<Model> model, glm::vec4 outlineColor) const;
};
#endif
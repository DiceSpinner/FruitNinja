#ifndef GAME_H
#define GAME_H
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <unordered_map>
#include <list>
#include <typeindex>
#include "../core/object.hpp"
#include "../rendering/model.hpp"
#include "../audio/audio_clip.hpp"

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

class Game;

class GameMode {
protected:
	Game* game;
public:
	GameMode(Game* game);
	virtual void Init() = 0;
	virtual void OnEnter() = 0;
	virtual void OnExit() = 0;
	virtual std::optional<std::type_index> Step() = 0;
	virtual void DrawVFX(Shader& vfxShader) = 0;
	virtual void DrawBackUI(Shader& uiShader) = 0;
	virtual void DrawFrontUI(Shader& uiShader) = 0;
};

struct MouseTrailSetting {
	size_t maxMousePosSize = 100;
	size_t maxTrailSize = 20;
	float arrowSize = 50;
	float mousePosKeepTime = 0.3f;
};

class Game {
private:
	std::unordered_map<std::type_index, std::unique_ptr<GameMode>> gameModes;
	GameMode* currMode;
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
	std::shared_ptr<Object> player;
	GameModels models;
	GameAudios audios;
	GameTextures textures;

	// Mouse Trail
	bool enableMouseTrail = true;

	Game();
	~Game();
	void Init();

	template<typename Mode>
	void AddGameMode() {
		auto type = std::type_index(typeid(Mode));
		auto mode = gameModes.find(type);
		if (mode != gameModes.end()) {
			return;
		}
		auto& inserted = gameModes.emplace(type, std::make_unique<Mode>(this)).first->second;
		inserted->Init();

		if(!currMode){
			currMode = inserted.get();
			currMode->OnEnter();
		}
	}

	template<typename... Modes>
	void AddGameModes() {
		(AddGameMode<Modes>(), ...);
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
};
#endif
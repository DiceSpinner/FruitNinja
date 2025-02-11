#include <memory>
#include <random>
#include <iostream>
#include <functional>
#include "../core/object.hpp"
#include "../core/object_pool.hpp"
#include "game.hpp"
#include "fruit.hpp"
#include "../audio/audio_clip.hpp"
#include "../settings/fruitsize.hpp"
#include "../settings/fruitspawn.hpp"
#include "../audio/audiosource_pool.hpp"
#include "../audio/audiolistener.hpp"
#include "../physics/rigidbody.hpp"
#include "../rendering/renderer.hpp"
#include "../rendering/camera.hpp"
#include "../rendering/model.hpp"
#include "../rendering/particle_system.hpp"
#include "../state/time.hpp"
#include "../state/state.hpp"
#include "../state/window.hpp"
#include "frontUI.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace std;
using namespace Game;

// Resource path
static const char* unitCube = "models/unit_cube.obj";
static const char* unitSphere = "models/unit_sphere.obj";

static const char* apple = "models/fruits/apple.obj";
static const char* appleTop = "models/fruits/apple_top.obj";
const char* appleBottom = "models/fruits/apple_bottom.obj";

static const char* pineapple = "models/fruits/pineapple.obj";
static const char* pineappleTop = "models/fruits/pineapple_top.obj";
static const char* pineappleBottom = "models/fruits/pineapple_bottom.obj";

static const char* watermelon = "models/fruits/watermelon.obj";
static const char* watermelonTop = "models/fruits/watermelon_top.obj";
static const char* watermelonBottom = "models/fruits/watermelon_bottom.obj";

// Audio
static shared_ptr<AudioClip> fruitSliceAudio;
static shared_ptr<AudioClip> largeFruitSliceAudio;
static shared_ptr<AudioClip> startMenuAudio;
static shared_ptr<AudioClip> gameStartAudio;
static shared_ptr<AudioClip> gameOverAudio;
static shared_ptr<AudioClip> fruitMissAudio;
static shared_ptr<AudioClip> fruitSpawnAudio;
static shared_ptr<AudioClip> recoveryAudio;

static void loadAudio() {
	fruitSliceAudio = make_shared<AudioClip>("sounds/onhit.wav");
	largeFruitSliceAudio = make_shared<AudioClip>("sounds/onhit2.wav");
	startMenuAudio = make_shared<AudioClip>("sounds/StartMenu.wav");
	gameStartAudio = make_shared<AudioClip>("sounds/Game-start.wav");
	gameOverAudio = make_shared<AudioClip>("sounds/Game-over.wav");
	fruitMissAudio = make_shared<AudioClip>("sounds/gank.wav");
	fruitSpawnAudio = make_shared<AudioClip>("sounds/Throw-fruit.wav");
	recoveryAudio = make_shared<AudioClip>("sounds/extra-life.wav");
}

// Models
static shared_ptr<Model> appleModel;
static shared_ptr<Model> appleTopModel;
static shared_ptr<Model> appleBottomModel;

static shared_ptr<Model> pineappleModel;
static shared_ptr<Model> pineappleTopModel;
static shared_ptr<Model> pineappleBottomModel;

static shared_ptr<Model> watermelonModel;
static shared_ptr<Model> watermelonTopModel;
static shared_ptr<Model> watermelonBottomModel;

static void loadModels() {
	appleModel = make_shared<Model>(apple);
	appleTopModel = make_shared<Model>(appleTop);
	appleBottomModel = make_shared<Model>(appleBottom);

	pineappleModel = make_shared<Model>(pineapple);
	pineappleTopModel = make_shared<Model>(pineappleTop);
	pineappleBottomModel = make_shared<Model>(pineappleBottom);

	watermelonModel = make_shared<Model>(watermelon);
	watermelonTopModel = make_shared<Model>(watermelonTop);
	watermelonBottomModel = make_shared<Model>(watermelonBottom);
}

static GLuint sliceParticleTexture;
static void loadTextures() {
	sliceParticleTexture = textureFromFile("droplet.png", "images");
}

static ObjectPool<Object>* fruitSliceParticlePool;
static Object* createFruitParticle() {
	Object* obj = new Object();
	ParticleSystem* system = obj->AddComponent<ParticleSystem>(100);
	system->disableOnFinish = true;
	system->maxSpawnDirectionDeviation = 180;
	system->spawnAmount = 22;
	system->spawnDirection = glm::vec3(0, 25, 0);
	system->scale = glm::vec3(0.3, 0.3, 0.3);
	system->spawnFrequency = 0;
	system->useGravity = true;

	return obj;
}

static float randFloat(float min, float max) {
	return rand() / static_cast<float>(RAND_MAX) * (max - min) + min;
}

static void spawnFruit(shared_ptr<Model>& fruitModel, shared_ptr<Model>& slice1Model, shared_ptr<Model>& slice2Model, shared_ptr<AudioClip> sliceAudio, glm::vec4 color, float radius=1, int score = 1) {
	shared_ptr<Object> fruit = Object::Create();

	auto renderer = fruit->AddComponent<Renderer>(fruitModel);
	renderer->drawOverlay = true;
	Fruit* ft = fruit->AddComponent<Fruit>(radius, score, *fruitSliceParticlePool, slice1Model, slice2Model, sliceAudio, fruitMissAudio);
	ft->slicedParticleTexture = sliceParticleTexture;
	ft->color = color;
	Rigidbody* rb = fruit->AddComponent<Rigidbody>();
	float upForce = randFloat(FRUIT_UP_MIN, FRUIT_UP_MAX);
	float horizontalForce = randFloat(FRUIT_HORIZONTAL_MIN, FRUIT_HORIZONTAL_MAX);
	rb->AddForce(glm::vec3(horizontalForce, upForce, 0), ForceMode::Impulse);

	glm::vec3 torque(
		randFloat(SPAWN_X_ROT_MIN, SPAWN_X_ROT_MAX),
		randFloat(SPAWN_Y_ROT_MIN, SPAWN_Y_ROT_MAX),
		randFloat(SPAWN_Z_ROT_MIN, SPAWN_Z_ROT_MAX)
	);

	rb->AddRelativeTorque(torque, ForceMode::Impulse);
	float startX =  randFloat(FRUIT_SPAWN_CENTER - FRUIT_SPAWN_WIDTH / 2, FRUIT_SPAWN_CENTER + FRUIT_SPAWN_WIDTH / 2);
	glm::vec3 position(startX, FRUIT_SPAWN_HEIGHT, 0);
	// cout << "Spawn at " << glm::to_string(position) << "\n";
	rb->transform.SetPosition(position);

	auto audioSourceObj = acquireAudioSource();
	if (audioSourceObj) {
		auto source = audioSourceObj->GetComponent<AudioSource>();
		source->SetAudioClip(fruitSpawnAudio);
		source->Play();
	}
}

static void spawnApple() {
	spawnFruit(appleModel, appleTopModel, appleBottomModel, fruitSliceAudio, glm::vec4(1, 1, 1, 1), APPLE_SIZE);
}

static void spawnPineapple() {
	spawnFruit(pineappleModel, pineappleTopModel, pineappleBottomModel, fruitSliceAudio, glm::vec4(1, 1, 0, 1), PINEAPPLE_SIZE);
}

static void spawnWatermelon() {
	spawnFruit(watermelonModel, watermelonTopModel, watermelonBottomModel, largeFruitSliceAudio, glm::vec4(1, 0, 0, 1), WATERMELON_SIZE);
}

static vector<function<void()>> spawners = {spawnApple, spawnPineapple, spawnWatermelon};

static float spawnCooldown = 1;
static float spawnTimer = 0;

static shared_ptr<Object> startGame;
static shared_ptr<Object> exitGame;
static shared_ptr<Object> restart;

static shared_ptr<Object> camera;
static shared_ptr<Object> particle;

void initGame() {
	loadAudio();
	loadTextures();
	loadModels();

	fruitSliceParticlePool = new ObjectPool<Object>(50, createFruitParticle);

	state = State::START;

	// Start Game Button
	startGame = Object::Create();
	startGame->AddComponent<Renderer>(watermelonModel);
	startGame->AddComponent<Fruit>(WATERMELON_SIZE, 0, *fruitSliceParticlePool, watermelonTopModel, watermelonBottomModel);
	Rigidbody* rigidbody = startGame->AddComponent<Rigidbody>();
	rigidbody->useGravity = false;
	glm::vec3 torque(
		randFloat(SPAWN_X_ROT_MIN, SPAWN_X_ROT_MAX),
		randFloat(SPAWN_Y_ROT_MIN, SPAWN_Y_ROT_MAX),
		randFloat(SPAWN_Z_ROT_MIN, SPAWN_Z_ROT_MAX)
	);
	rigidbody->AddRelativeTorque(torque, ForceMode::Impulse);

	// Exit button
	exitGame = Object::Create();
	exitGame->AddComponent<Renderer>(appleModel);
	exitGame->AddComponent<Fruit>(APPLE_SIZE, 0, *fruitSliceParticlePool, appleTopModel, appleBottomModel);
	rigidbody = exitGame->AddComponent<Rigidbody>();
	rigidbody->useGravity = false;
	torque = glm::vec3(
		randFloat(SPAWN_X_ROT_MIN, SPAWN_X_ROT_MAX),
		randFloat(SPAWN_Y_ROT_MIN, SPAWN_Y_ROT_MAX),
		randFloat(SPAWN_Z_ROT_MIN, SPAWN_Z_ROT_MAX)
	);
	rigidbody->AddRelativeTorque(torque, ForceMode::Impulse);
	
	// Reset Button
	restart = Object::Create();
	restart->SetEnable(false);
	restart->AddComponent<Renderer>(pineappleModel);
	restart->AddComponent<Fruit>(PINEAPPLE_SIZE, 0, *fruitSliceParticlePool, pineappleTopModel, pineappleBottomModel);
	rigidbody = restart->AddComponent<Rigidbody>();
	rigidbody->useGravity = false;
	torque = glm::vec3(
		randFloat(SPAWN_X_ROT_MIN, SPAWN_X_ROT_MAX),
		randFloat(SPAWN_Y_ROT_MIN, SPAWN_Y_ROT_MAX),
		randFloat(SPAWN_Z_ROT_MIN, SPAWN_Z_ROT_MAX)
	);
	rigidbody->AddRelativeTorque(torque, ForceMode::Impulse);

	// Audio & Camera
	camera = Object::Create();
	camera->AddComponent<Camera>(1.0f, 300.0f);
	camera->transform.SetPosition(glm::vec3(0, 0, 30));
	camera->transform.LookAt(camera->transform.position() + glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
	camera->AddComponent<AudioListener>();

	AudioSource* audioSource = camera->AddComponent<AudioSource>();
	audioSource->SetAudioClip(startMenuAudio);
	audioSource->SetLoopEnabled(true);
	audioSource->Play();
	alDistanceModel(AL_NONE);

	// Set Button position
	glm::mat4 inverse = glm::inverse(Camera::main->Perspective() * Camera::main->View());
	float z = Camera::main->ComputerNormalizedZ(30);
	glm::vec4 startPos = inverse * glm::vec4(START_BUTTON_POS, z, 1);
	startPos /= startPos.w;
	startGame->transform.SetPosition(startPos);

	glm::vec4 exitPos = inverse * glm::vec4(EXIT_BUTTON_POS, z, 1);
	exitPos /= exitPos.w;
	exitGame->transform.SetPosition(exitPos);
}

static void enterGame() {
	state = State::GAME;
	exitGame->GetComponent<Rigidbody>()->useGravity = true;
	score = 0;
	spawnTimer = 0;
	misses = 0;
	recovery = 0;
	camera->GetComponent<AudioSource>()->Pause();
	auto source = acquireAudioSource();
	if (source) {
		auto audioSource = source->GetComponent<AudioSource>();
		audioSource->SetAudioClip(gameStartAudio);
		audioSource->Play();
	}
}

static void enterScore() {
	state = State::SCORE;
	exitGame->SetEnable(true);
	auto rb = exitGame->GetComponent<Rigidbody>();
	rb->useGravity = false;
	rb->velocity = glm::vec3(0);
	restart->SetEnable(true);

	glm::mat4 inverse = glm::inverse(Camera::main->Perspective() * Camera::main->View());
	float z = Camera::main->ComputerNormalizedZ(30);
	glm::vec4 startPos = inverse * glm::vec4(START_BUTTON_POS, z, 1);
	startPos /= startPos.w;
	restart->transform.SetPosition(startPos);

	glm::vec4 exitPos = inverse * glm::vec4(EXIT_BUTTON_POS, z, 1);
	exitPos /= exitPos.w;
	exitGame->transform.SetPosition(exitPos);

	auto source = acquireAudioSource();
	if (source) {
		auto audioSource = source->GetComponent<AudioSource>();
		audioSource->SetAudioClip(gameOverAudio);
		audioSource->Play();
	}
}

static void processStart() {
	if (!startGame->IsActive()) {
		enterGame();
	}
	else if (!exitGame->IsActive()) {
		glfwSetWindowShouldClose(window, true);
	}
}

static void processGame() {
	spawnTimer += deltaTime();
	if (spawnTimer >= spawnCooldown) {
		int spawnAmount = round(randFloat(SPAWN_AMOUNT_MIN, SPAWN_AMOUNT_MAX));
		for (int i = 0; i < spawnAmount; i++) {
			int index = round(randFloat(0, spawners.size() - 1));
			spawners[index]();
		}
		spawnCooldown = round(randFloat(SPAWN_COOLDOWN_MIN, SPAWN_COOLDOWN_MAX));
		spawnTimer = 0;
	}
	if (misses >= MISS_TOLERENCE) {
		enterScore();
	}
	if (recentlyRecovered) {
		recentlyRecovered = false;
		auto source = acquireAudioSource();
		if (source) {
			auto audioSource = source->GetComponent<AudioSource>();
			audioSource->SetAudioClip(recoveryAudio);
			audioSource->Play();
		}
	}
}

static void processScore() {
	if (!restart->IsActive()) {
		enterGame();
	}
	else if (!exitGame->IsActive()) {
		glfwSetWindowShouldClose(window, true);
	}
}

void gameStep() {
	switch (state) {
		case State::START:
			processStart();
			break;
		case State::GAME:
			processGame();
			break;
		case State::SCORE:
			processScore();
			break;
	}
}
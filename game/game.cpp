#include <memory>
#include <random>
#include <iostream>
#include <functional>
#include "../core/object.hpp"
#include "game.hpp"
#include "fruit.hpp"
#include "../audio/audio_clip.hpp"
#include "../settings/fruitsize.hpp"
#include "../settings/fruitspawn.hpp"
#include "../audio/audiosource.hpp"
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

static const char* fruitSliceAudioPath = "sounds/hit2.wav";
static const char* bgmPath = "sounds/abyss.wav";

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
static shared_ptr<AudioClip> bgmAudio;

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

static float randFloat(float min, float max) {
	return rand() / static_cast<float>(RAND_MAX) * (max - min) + min;
}

static void spawnFruit(shared_ptr<Model>& fruitModel, shared_ptr<Model>& slice1Model, shared_ptr<Model>& slice2Model, float radius=1, int score = 1) {
	shared_ptr<Object> fruit = Object::Create();
	auto audioSource = fruit->AddComponent<AudioSource>();
	audioSource->SetAudioClip(fruitSliceAudio);
	audioSource->delayDeletionUntilFinish = true;

	auto renderer = fruit->AddComponent<Renderer>(fruitModel);
	renderer->drawOverlay = true;
	fruit->AddComponent<Fruit>(radius, score, slice1Model, slice2Model);
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
}

static void spawnApple() {
	spawnFruit(appleModel, appleTopModel, appleBottomModel, APPLE_SIZE);
}

static void spawnPineapple() {
	spawnFruit(pineappleModel, pineappleTopModel, pineappleBottomModel, PINEAPPLE_SIZE);
}

static void spawnWatermelon() {
	spawnFruit(watermelonModel, watermelonTopModel, watermelonBottomModel, WATERMELON_SIZE);
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
	fruitSliceAudio = make_shared<AudioClip>(fruitSliceAudioPath);
	bgmAudio = make_shared<AudioClip>(bgmPath);

	appleModel = make_shared<Model>(apple);
	appleTopModel = make_shared<Model>(appleTop);
	appleBottomModel = make_shared<Model>(appleBottom);

	pineappleModel = make_shared<Model>(pineapple);
	pineappleTopModel = make_shared<Model>(pineappleTop);
	pineappleBottomModel = make_shared<Model>(pineappleBottom);

	watermelonModel = make_shared<Model>(watermelon);
	watermelonTopModel = make_shared<Model>(watermelonTop);
	watermelonBottomModel = make_shared<Model>(watermelonBottom);

	state = State::START;

	particle = Object::Create();
	ParticleSystem* system = particle->AddComponent<ParticleSystem>(300, 3);
	system->useGravity = false;
	GLuint image = textureFromFile("awesomeface.png", "images");
	system->SetTexture(image);

	// Start Game Button
	startGame = Object::Create();
	startGame->AddComponent<Renderer>(watermelonModel);
	startGame->AddComponent<Fruit>(WATERMELON_SIZE, 0, watermelonTopModel, watermelonBottomModel);
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
	exitGame->AddComponent<Fruit>(APPLE_SIZE, 0, appleTopModel, appleBottomModel);
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
	restart->AddComponent<Fruit>(PINEAPPLE_SIZE, 0, pineappleTopModel, pineappleBottomModel);
	rigidbody = restart->AddComponent<Rigidbody>();
	rigidbody->useGravity = false;
	torque = glm::vec3(
		randFloat(SPAWN_X_ROT_MIN, SPAWN_X_ROT_MAX),
		randFloat(SPAWN_Y_ROT_MIN, SPAWN_Y_ROT_MAX),
		randFloat(SPAWN_Z_ROT_MIN, SPAWN_Z_ROT_MAX)
	);
	rigidbody->AddRelativeTorque(torque, ForceMode::Impulse);

	camera = Object::Create();
	camera->AddComponent<Camera>(1.0f, 300.0f);
	camera->transform.SetPosition(glm::vec3(0, 0, 30));
	camera->transform.LookAt(camera->transform.position() + glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
	camera->AddComponent<AudioListener>();

	AudioSource* audioSource = camera->AddComponent<AudioSource>();
	audioSource->SetAudioClip(bgmAudio);
	audioSource->SetLoopEnabled(true);
	audioSource->Play();
	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

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
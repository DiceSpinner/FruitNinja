#include <memory>
#include <random>
#include <iostream>
#include <functional>
#include "game.hpp"
#include "../settings/fruitsize.hpp"
#include "../settings/fruitplane.hpp"
#include "../components/fruit.hpp"
#include "../components/rigidbody.hpp"
#include "../rendering/model.hpp"
#include "../state/time.hpp"
#include "../state/new_objects.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace std;

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
	shared_ptr<Object> fruit = make_shared<Object>(fruitModel);
	fruit->drawOverlay = true;
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
	cout << "Spawn at " << glm::to_string(position) << "\n";
	rb->transform.SetPosition(position);
	GameState::newObjects.push(fruit);
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

void initGame() {
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

void step() {
	spawnTimer += deltaTime();
	if (spawnTimer >= spawnCooldown) {
		int spawnAmount = round(randFloat(SPAWN_AMOUNT_MIN, SPAWN_AMOUNT_MAX));
		for (int i = 0; i < spawnAmount;i++) {
			int index = round(randFloat(0, spawners.size() - 1));
			spawners[index]();
		}
		spawnCooldown = round(randFloat(SPAWN_COOLDOWN_MIN, SPAWN_COOLDOWN_MAX));
		spawnTimer = 0;
	}
}
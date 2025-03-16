#include <memory>
#include <random>
#include <iostream>
#include <functional>
#include "infrastructure/object.hpp"
#include "infrastructure/object_pool.hpp"
#include "game.hpp"
#include "fruit.hpp"
#include "bomb.hpp"
#include "audio/audio_clip.hpp"
#include "audio/audiosource_pool.hpp"
#include "audio/audiolistener.hpp"
#include "physics/rigidbody.hpp"
#include "rendering/renderer.hpp"
#include "rendering/camera.hpp"
#include "rendering/model.hpp"
#include "rendering/particle_system.hpp"
#include "state/cursor.hpp"
#include "state/time.hpp"
#include "state/window.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace std;

// Given three collinear points p, q, r, the function checks if 
// point q lies on line segment 'pr' 
static bool onSegment(glm::vec2 p, glm::vec2 q, glm::vec2 r)
{
	if (q.x <= max(p.x, r.x) && q.x >= min(p.x, r.x) &&
		q.y <= max(p.y, r.y) && q.y >= min(p.y, r.y))
		return true;

	return false;
}

// To find orientation of ordered triplet (p, q, r). 
// The function returns following values 
// 0 --> p, q and r are collinear 
// 1 --> Clockwise 
// 2 --> Counterclockwise 
static int orientation(glm::vec2 p, glm::vec2 q, glm::vec2 r)
{
	// See https://www.geeksforgeeks.org/orientation-3-ordered-points/ 
	// for details of below formula. 
	int val = (q.y - p.y) * (r.x - q.x) -
		(q.x - p.x) * (r.y - q.y);

	if (val == 0) return 0;  // collinear 

	return (val > 0) ? 1 : 2; // clock or counterclock wise 
}

// The main function that returns true if line segment 'p1q1' 
// and 'p2q2' intersect. 
static bool doIntersect(glm::vec2 p1, glm::vec2 q1, glm::vec2 p2, glm::vec2 q2)
{
	// Find the four orientations needed for general and 
	// special cases 
	int o1 = orientation(p1, q1, p2);
	int o2 = orientation(p1, q1, q2);
	int o3 = orientation(p2, q2, p1);
	int o4 = orientation(p2, q2, q1);

	// General case 
	if (o1 != o2 && o3 != o4)
		return true;

	// Special Cases 
	// p1, q1 and p2 are collinear and p2 lies on segment p1q1 
	if (o1 == 0 && onSegment(p1, p2, q1)) return true;

	// p1, q1 and q2 are collinear and q2 lies on segment p1q1 
	if (o2 == 0 && onSegment(p1, q2, q1)) return true;

	// p2, q2 and p1 are collinear and p1 lies on segment p2q2 
	if (o3 == 0 && onSegment(p2, p1, q2)) return true;

	// p2, q2 and q1 are collinear and q1 lies on segment p2q2 
	if (o4 == 0 && onSegment(p2, q1, q2)) return true;

	return false; // Doesn't fall in any of the above cases 
}

static void insertTrailArrow(char* currVBO, int* currEBO, glm::vec2 left, glm::vec2 right, glm::vec2 tangent, int index, MouseTrailSetting& trailSetting) {
	glm::vec3 backLeftPos(left, -0.8f);
	glm::vec3 backRightPos(right, -0.8f);
	glm::vec2 backLeftUV(0, 1);
	memcpy(currVBO, glm::value_ptr(backLeftPos), sizeof(backLeftPos));
	currVBO += sizeof(glm::vec3);
	memcpy(currVBO, glm::value_ptr(backLeftUV), sizeof(backLeftUV));
	currVBO += sizeof(glm::vec2);

	glm::vec2 backRightUV(0, 0);
	memcpy(currVBO, glm::value_ptr(backRightPos), sizeof(backRightPos));
	currVBO += sizeof(glm::vec3);
	memcpy(currVBO, glm::value_ptr(backRightUV), sizeof(backRightUV));
	currVBO += sizeof(glm::vec2);

	auto offset = trailSetting.arrowSize * glm::normalize(glm::vec3(tangent, 0));

	glm::vec3 frontLeftPos = backLeftPos + offset;
	glm::vec2 frontLeftUV(1, 1);
	memcpy(currVBO, glm::value_ptr(frontLeftPos), sizeof(frontLeftPos));
	currVBO += sizeof(glm::vec3);
	memcpy(currVBO, glm::value_ptr(frontLeftUV), sizeof(frontLeftUV));
	currVBO += sizeof(glm::vec2);

	glm::vec3 frontRightPos = backRightPos + offset;
	glm::vec2 frontRightUV(1, 0);
	memcpy(currVBO, glm::value_ptr(frontRightPos), sizeof(frontRightPos));
	currVBO += sizeof(glm::vec3);
	memcpy(currVBO, glm::value_ptr(frontRightUV), sizeof(frontRightUV));
	currVBO += sizeof(glm::vec2);
	currEBO[0] = index;
	currEBO[1] = index + 1;
	currEBO[2] = index + 2;
	currEBO[3] = index + 3;
	currEBO[4] = index + 2;
	currEBO[5] = index + 1;
}

static float randFloat(float min, float max) {
	return rand() / static_cast<float>(RAND_MAX) * (max - min) + min;
}

GameState::GameState(Game* game) : game(game), transition(), terminated(false), enterSubState() {}
void GameState::Terminate() { terminated = true; OnExit(); }
void GameState::DrawBackUI(Shader& uiShader) {
	if (!currSubState) { OnDrawBackUI(uiShader); return; }
	if (currSubState->AllowParentBackUI()) OnDrawBackUI(uiShader);
	currSubState->DrawBackUI(uiShader);
}
void GameState::DrawFrontUI(Shader& uiShader) {
	if (!currSubState) { OnDrawFrontUI(uiShader); return; }
	if (currSubState->AllowParentFrontUI()) OnDrawFrontUI(uiShader);
	currSubState->DrawFrontUI(uiShader);
}
void GameState::DrawVFX(Shader& vfxShader){
	if (!currSubState) { OnDrawVFX(vfxShader); return; }
	currSubState->DrawVFX(vfxShader);
}
void GameState::OnEnterSubState() {}
void GameState::OnExitSubState() {}
void GameState::OnDrawFrontUI(Shader& uiShader) {}
void GameState::OnDrawBackUI(Shader& uiShader) {}
void GameState::OnDrawVFX(Shader& vfxShader) {}
bool GameState::AllowParentBackUI() { return true; }
bool GameState::AllowParentFrontUI() { return true; }
type_index GameState::Self() const { return type_index(typeid(*this)); }
void GameState::StartCoroutine(Coroutine&& coroutine) {
	coroutineManager.AddCoroutine(forward<Coroutine>(coroutine));
}
optional<type_index> GameState::Step() { return {}; };
optional<type_index> GameState::Run() {
	// Coroutines must be finished before states can be runned
	if (!coroutineManager.Empty()) {
		coroutineManager.Run();
		return Self();
	}
	if (terminated) { terminated = false; return transition; }

	if (enterSubState) {
		currSubState = std::exchange(enterSubState, {});
		currSubState->OnEnter();
	}

	// Run current state if there's no substate
	if (!currSubState) {
		auto next = Step();
		if (!next || next.value() != Self()) { 
			transition = next;
			Terminate(); return Self(); 
		}
		return next;
	}
	
	// Run sub state
	auto next = currSubState->Run();
	if (!next) {
		currSubState = {};
		OnExitSubState();
	}
	else if(next.value() != type_index(typeid(*currSubState))) {
		auto result = states.find(next.value());
		if (result != states.end()) {
			currSubState = result->second.get();
			currSubState->OnEnter();
		}
		else {
			cout << "State not found" << endl;
		}
	}
	return Self();
}

Game::Game() : gameState(), textures() {
	trailShader = make_unique<Shader>("shaders/mouse_trail.vert", "shaders/mouse_trail.frag");
	trailTexture = textureFromFile("FruitNinja_blade0.png", "images");
	trailArrow = textureFromFile("blade0_arrow.png", "images");
	player = Object::Create();

	// Configure mouse trailing (shared by all game modes)
	Cursor::OnMousePositionUpdated.push_back(bind(&Game::OnMouseUpdate, this, placeholders::_1));
	Cursor::OnLeftClickReleased.push_back(bind(&Game::OnLeftReleased, this));
	glGenBuffers(1, &mouseTrailVBO);
	glGenBuffers(1, &mouseTrailEBO);
	glGenVertexArrays(1, &mouseTrailVAO);

	glBindVertexArray(mouseTrailVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mouseTrailVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mouseTrailEBO);

	glBufferData(GL_ARRAY_BUFFER, 4 * mouseTrailSetting.maxMousePosSize * 5 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mouseTrailSetting.maxMousePosSize * 6 * sizeof(int), nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)sizeof(glm::vec3));
	glBindVertexArray(0);
}

Game::~Game() {
	glDeleteVertexArrays(1, &mouseTrailVAO);
	glDeleteBuffers(1, &mouseTrailVBO);
	glDeleteBuffers(1, &mouseTrailEBO);
}

void Game::OnMouseUpdate(glm::vec2 position) {
	if (Cursor::mouseClicked && enableMouseTrail) {
		if (flushMousePositions) {
			flushMousePositions = false;
			mousePositions.clear();
		}
		glm::vec2 expanded(position.x * SCR_WIDTH / 2, position.y * SCR_HEIGHT / 2);
		if (!mousePositions.empty()) {
			glm::vec2 lastPos = mousePositions.back();
			if (glm::length(expanded - lastPos) < 12) {
				return;
			}
		}

		mousePositions.push_back(glm::vec3(expanded, mouseTrailSetting.mousePosKeepTime));
		if (mousePositions.size() > mouseTrailSetting.maxMousePosSize) {
			mousePositions.pop_front();
		}
	}
	if (!enableMouseTrail) {
		mousePositions.clear();
	}
}

void Game::OnLeftReleased() {
	flushMousePositions = true;
}

void Game::Init() {
	LoadAudios();
	LoadTextures();
	LoadModels();

	uiConfig.control.particleSystemPool = make_unique<ObjectPool<Object>>(50, createFruitParticleSystem);
	uiConfig.control.killHeight = uiConfig.fruitKillHeight;
}

void Game::LoadAudios() {
	audios.fruitSliceAudio1 = make_shared<AudioClip>("sounds/onhit.wav");
	audios.fruitSliceAudio2 = make_shared<AudioClip>("sounds/onhit2.wav");
	audios.startMenuAudio = make_shared<AudioClip>("sounds/StartMenu.wav");
	audios.gameStartAudio = make_shared<AudioClip>("sounds/Game-start.wav");
	audios.gameOverAudio = make_shared<AudioClip>("sounds/Game-over.wav");
	audios.fruitMissAudio = make_shared<AudioClip>("sounds/gank.wav");
	audios.fruitSpawnAudio = make_shared<AudioClip>("sounds/Throw-fruit.wav");
	audios.recoveryAudio = make_shared<AudioClip>("sounds/extra-life.wav");
	audios.explosionAudio = make_shared<AudioClip>("sounds/Bomb-explode.wav");
	audios.fuseAudio = make_shared<AudioClip>("sounds/Bomb-Fuse.wav");
}

void Game::LoadModels() {
	models.bombModel = make_shared<Model>("models/bomb.obj");

	models.appleModel = make_shared<Model>("models/fruits/apple.obj");
	models.appleTopModel = make_shared<Model>("models/fruits/apple_top.obj");
	models.appleBottomModel = make_shared<Model>("models/fruits/apple_bottom.obj");

	models.pineappleModel = make_shared<Model>("models/fruits/pineapple.obj");
	models.pineappleTopModel = make_shared<Model>("models/fruits/pineapple_top.obj");
	models.pineappleBottomModel = make_shared<Model>("models/fruits/pineapple_bottom.obj");

	models.watermelonModel = make_shared<Model>("models/fruits/watermelon.obj");
	models.watermelonTopModel = make_shared<Model>("models/fruits/watermelon_top.obj");
	models.watermelonBottomModel = make_shared<Model>("models/fruits/watermelon_bottom.obj");

	models.coconutModel = make_shared<Model>("models/fruits/coconut.obj");
	models.coconutTopModel = make_shared<Model>("models/fruits/coconut_top.obj");
	models.coconutBottomModel = make_shared<Model>("models/fruits/coconut_bottom.obj");
}

void Game::LoadTextures() {
	textures.sliceParticleTexture = textureFromFile("droplet.png", "images");
	textures.sparkTexture = textureFromFile("spark.png", "images");
	textures.smokeTexture = textureFromFile("smoke3.png", "images");
	textures.backgroundTexture = textureFromFile("fruit_ninja_clean.png", "images");
	textures.fadeTexture = textureFromFile("fading_circle_opaque_center.png", "images");
	textures.emptyCross = textureFromFile("empty_cross.png", "images");
	textures.redCross = textureFromFile("red_cross.png", "images");
}

void Game::Step() {
	if (!gameState->Run()) {
		glfwSetWindowShouldClose(window, true);
	}
}
	
void Game::DrawVFX(Shader& vfxShader) {
	if (gameState) {
		gameState->DrawVFX(vfxShader);
	}
}

void Game::DrawBackUI(Shader& uiShader) {
	if (gameState) {
		gameState->DrawBackUI(uiShader);
	}
}

void Game::DrawFrontUI(Shader& uiShader) {
	if (gameState) {
		gameState->DrawFrontUI(uiShader);
	}
	trailShader->Use();
	DrawMouseTrailing();
	uiShader.Use();
}

void Game::DrawMouseTrailing() {
	float dt = Time::deltaTime();
	for (auto i = mousePositions.begin(); i != mousePositions.end();) {
		i->z -= dt;
		if (i->z <= 0) {
			i = mousePositions.erase(i);
		}
		else {
			i++;
		}
	}

	glBindVertexArray(mouseTrailVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mouseTrailVBO);
	if (mousePositions.size() > 1) {
		if (mousePositions.size() > 10) {
			cout << "";
		}
		char* currVBO = (char*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		int* currEBO = (int*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

		auto prev = mousePositions.begin();
		auto curr = next(prev, 1);
		glm::vec2 tangent{};
		int index = 0;
		int indices = 6;
		glm::vec3 backLeftPos;
		glm::vec3 backRightPos;
		bool init = true;
		while (curr != mousePositions.end()) {
			glm::vec2 prevPos(*prev);
			glm::vec2 currPos(*curr);
			tangent = currPos - prevPos;

			glm::vec2 right = glm::normalize(glm::vec2(tangent.y, -tangent.x));
			float prevLineSize = prev->z * mouseTrailSetting.maxTrailSize / mouseTrailSetting.mousePosKeepTime;
			float currLineSize = curr->z * mouseTrailSetting.maxTrailSize / mouseTrailSetting.mousePosKeepTime;

			glm::vec2 prevRight = prevLineSize * right;
			glm::vec2 prevLeft = -prevLineSize * right;
			glm::vec2 currRight = currLineSize * right;
			glm::vec2 currLeft = -currLineSize * right;

			if (init) {
				init = false;
				backLeftPos = glm::vec3(prevPos + prevLeft, -0.5);
				backRightPos = glm::vec3(prevPos + prevRight, -0.5);
			}

			glm::vec2 backLeftUV(0, 1);
			memcpy(currVBO, glm::value_ptr(backLeftPos), sizeof(backLeftPos));
			currVBO += sizeof(glm::vec3);
			memcpy(currVBO, glm::value_ptr(backLeftUV), sizeof(backLeftUV));
			currVBO += sizeof(glm::vec2);

			glm::vec2 backRightUV(0, 0);
			memcpy(currVBO, glm::value_ptr(backRightPos), sizeof(backRightPos));
			currVBO += sizeof(glm::vec3);
			memcpy(currVBO, glm::value_ptr(backRightUV), sizeof(backRightUV));
			currVBO += sizeof(glm::vec2);

			glm::vec3 frontLeftPos(currPos + currLeft, -0.5);
			glm::vec2 frontLeftUV(1, 1);
			memcpy(currVBO, glm::value_ptr(frontLeftPos), sizeof(frontLeftPos));
			currVBO += sizeof(glm::vec3);
			memcpy(currVBO, glm::value_ptr(frontLeftUV), sizeof(frontLeftUV));
			currVBO += sizeof(glm::vec2);

			glm::vec3 frontRightPos(currPos + currRight, -0.5);
			glm::vec2 frontRightUV(1, 0);
			memcpy(currVBO, glm::value_ptr(frontRightPos), sizeof(frontRightPos));
			currVBO += sizeof(glm::vec3);
			memcpy(currVBO, glm::value_ptr(frontRightUV), sizeof(frontRightUV));
			currVBO += sizeof(glm::vec2);

			if (doIntersect(backLeftPos, backRightPos, frontLeftPos, frontRightPos)) {
				currEBO[0] = index;
				currEBO[1] = index + 1;
				currEBO[2] = index + 2;
				currEBO[3] = index + 1;
				currEBO[4] = index;
				currEBO[5] = index + 3;
			}
			else {
				currEBO[0] = index;
				currEBO[1] = index + 1;
				currEBO[2] = index + 2;
				currEBO[3] = index + 3;
				currEBO[4] = index + 2;
				currEBO[5] = index + 1;
			}
			backLeftPos = frontLeftPos;
			backRightPos = frontRightPos;

			currEBO += 6;
			indices += 6;
			curr++;
			prev++;
			index += 4;
		}
		insertTrailArrow(currVBO, currEBO, backLeftPos, backRightPos, tangent, index, mouseTrailSetting);

		glUnmapBuffer(GL_ARRAY_BUFFER);
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, trailTexture);
		trailShader->SetInt("image", 0);
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, trailArrow);
		trailShader->SetInt("arrow", 1);
		glUniformMatrix4fv(glGetUniformLocation(trailShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(Camera::main->Ortho()));
		glDrawElements(GL_TRIANGLES, indices, GL_UNSIGNED_INT, 0);
	}

	glBindVertexArray(0);
}

shared_ptr<Object> Game::createUIObject(std::shared_ptr<Model> model, glm::vec4 outlineColor) const {
	auto obj = Object::Create(false);
	Renderer* renderer = obj->AddComponent<Renderer>(model);
	renderer->drawOutline = true;
	renderer->outlineColor = outlineColor;

	Rigidbody* rigidbody = obj->AddComponent<Rigidbody>();
	rigidbody->useGravity = false;
	glm::vec3 torque(
		randFloat(uiConfig.spawnMinRotation.x, uiConfig.spawnMaxRotation.x),
		randFloat(uiConfig.spawnMinRotation.y, uiConfig.spawnMaxRotation.y),
		randFloat(uiConfig.spawnMinRotation.z, uiConfig.spawnMaxRotation.z)
	);
	rigidbody->AddRelativeTorque(torque, ForceMode::Impulse);
	return obj;
}

shared_ptr<Object> Game::createUIObject(std::shared_ptr<Model> model) const {
	auto obj = Object::Create(false);
	Renderer* renderer = obj->AddComponent<Renderer>(model);
	renderer->drawOutline = false;

	Rigidbody* rigidbody = obj->AddComponent<Rigidbody>();
	rigidbody->useGravity = false;
	glm::vec3 torque(
		randFloat(uiConfig.spawnMinRotation.x, uiConfig.spawnMaxRotation.x),
		randFloat(uiConfig.spawnMinRotation.y, uiConfig.spawnMaxRotation.y),
		randFloat(uiConfig.spawnMinRotation.z, uiConfig.spawnMaxRotation.z)
	);
	rigidbody->AddRelativeTorque(torque, ForceMode::Impulse);
	return obj;
}

Object* Game::createFruitParticleSystem() {
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
#include "classic.hpp"
#include "fruit.hpp"
#include "bomb.hpp"
#include "../state/time.hpp"
#include "../state/window.hpp"
#include "../physics/rigidbody.hpp"
#include "../audio/audiosource.hpp"
#include "../audio/audiolistener.hpp"
#include "../audio/audiosource_pool.hpp"
#include "../rendering/renderer.hpp"
#include "../rendering/particle_system.hpp"
#include "../rendering/camera.hpp"

using namespace std;

static float randFloat(float min, float max) {
	return rand() / static_cast<float>(RAND_MAX) * (max - min) + min;
}

ClassicMode::ClassicMode(Game* game) 
	: GameMode(game), 
	fruitSpawners {
		bind(&ClassicMode::spawnApple, this), bind(&ClassicMode::spawnPineapple, this), bind(&ClassicMode::spawnWatermelon, this), bind(&ClassicMode::spawnBomb, this)
	}
{
}

void ClassicMode::spawnFruit(shared_ptr<Model>& fruitModel, shared_ptr<Model>& slice1Model, shared_ptr<Model>& slice2Model, shared_ptr<AudioClip> sliceAudio, glm::vec4 color, float radius, int score) {
	shared_ptr<Object> fruit = Object::Create();
	FruitAsset asset = {
		slice1Model,
		slice2Model,
		sliceAudio,
		game->audios.fruitMissAudio
	};

	auto renderer = fruit->AddComponent<Renderer>(fruitModel);
	// renderer->drawOverlay = true;
	Fruit* ft = fruit->AddComponent<Fruit>(radius, score, setting.fruitSliceForce, state.fruitChannel, asset);
	ft->slicedParticleTexture = game->textures.sliceParticleTexture;
	ft->color = color;
	Rigidbody* rb = fruit->AddComponent<Rigidbody>();
	float upForce = randFloat(setting.fruitUpMin, setting.fruitUpMax);
	float horizontalForce = randFloat(setting.fruitHorizontalMin, setting.fruitHorizontalMax);
	rb->AddForce(glm::vec3(horizontalForce, upForce, 0), ForceMode::Impulse);

	glm::vec3 torque(
		randFloat(setting.spawnMinRotation.x, setting.spawnMaxRotation.x),
		randFloat(setting.spawnMinRotation.y, setting.spawnMaxRotation.y),
		randFloat(setting.spawnMinRotation.z, setting.spawnMaxRotation.z)
	);

	rb->AddRelativeTorque(torque, ForceMode::Impulse);
	float startX = randFloat(setting.fruitSpawnCenter - setting.fruitSpawnWidth / 2, setting.fruitSpawnCenter + setting.fruitSpawnWidth / 2);
	glm::vec3 position(startX, setting.fruitSpawnHeight, 0);
	// cout << "Spawn at " << glm::to_string(position) << "\n";
	rb->transform.SetPosition(position);
}

void ClassicMode::spawnApple() {
	spawnFruit(game->models.appleModel, game->models.appleTopModel, game->models.appleBottomModel, game->audios.fruitSliceAudio1, glm::vec4(1, 1, 1, 1), setting.sizeApple);
}

void ClassicMode::spawnPineapple() {
	spawnFruit(game->models.pineappleModel, game->models.pineappleTopModel, game->models.pineappleBottomModel, game->audios.fruitSliceAudio1, glm::vec4(1, 1, 0, 1), setting.sizePineapple);
}

void ClassicMode::spawnWatermelon() {
	spawnFruit(game->models.watermelonModel, game->models.watermelonTopModel, game->models.watermelonBottomModel, game->audios.fruitSliceAudio2, glm::vec4(1, 0, 0, 1), setting.sizeWatermelon);
}

static void sparkModifier(Particle& particle, ParticleSystem& system) {
	float quater = 0.1 * particle.lifeTime;
	float remainder = 0.9 * particle.lifeTime;
	float opacity = particle.timeLived < quater ? particle.timeLived / quater : (remainder - particle.timeLived + quater) / remainder;
	float size = particle.timeLived < quater ? particle.timeLived / quater : (remainder - particle.timeLived + quater) / remainder;
	particle.scale = glm::vec3(size, size, size);
	particle.color = system.color * glm::vec4(1, 1, 1, opacity);
}

static void smokeModifier(Particle& particle, ParticleSystem& system) {
	float quater = 0.1 * particle.lifeTime;
	float remainder = 0.9 * particle.lifeTime;
	float size = particle.timeLived < quater ? particle.timeLived / quater : 1;
	float opacity = particle.timeLived < quater ? particle.timeLived / quater : (remainder - particle.timeLived + quater) / remainder;
	particle.velocity = glm::vec3(0, 1, 0);
	particle.scale = size * glm::vec3(1.2, 1.2, 1);
	particle.color = system.color * glm::vec4(1, 1, 1, opacity);
}

void ClassicMode::spawnBomb() {
	shared_ptr<Object> bomb = Object::Create();
	bomb->transform.SetScale(0.7f * glm::vec3(1, 1, 1));

	auto renderer = bomb->AddComponent<Renderer>(game->models.bombModel);
	// renderer->drawOverlay = true;
	renderer->drawOutline = true;
	renderer->outlineColor = glm::vec4(1, 0, 0, 1);
	bomb->AddComponent<Bomb>(game->audios.explosionAudio, setting.sizeBomb, state.bombChannel);

	AudioSource* audioSource = bomb->AddComponent<AudioSource>();
	audioSource->SetAudioClip(game->audios.fuseAudio);
	audioSource->SetLoopEnabled(true);
	audioSource->Play();

	Rigidbody* rb = bomb->AddComponent<Rigidbody>();
	float upForce = randFloat(setting.fruitUpMin, setting.fruitUpMax);
	float horizontalForce = randFloat(setting.fruitHorizontalMin, setting.fruitHorizontalMax);
	rb->AddForce(glm::vec3(horizontalForce, upForce, 0), ForceMode::Impulse);

	glm::vec3 torque(
		randFloat(setting.spawnMinRotation.x, setting.spawnMaxRotation.x),
		randFloat(setting.spawnMinRotation.y, setting.spawnMaxRotation.y),
		randFloat(setting.spawnMinRotation.z, setting.spawnMaxRotation.z)
	);

	rb->AddRelativeTorque(torque, ForceMode::Impulse);
	float startX = randFloat(setting.fruitSpawnCenter - setting.fruitSpawnWidth / 2, setting.fruitSpawnCenter + setting.fruitSpawnWidth / 2);
	glm::vec3 position(startX, setting.fruitSpawnHeight, 5);
	// cout << "Spawn at " << glm::to_string(position) << "\n";
	rb->transform.SetPosition(position);

	auto audioSourceObj = acquireAudioSource();
	if (audioSourceObj) {
		auto source = audioSourceObj->GetComponent<AudioSource>();
		source->SetAudioClip(game->audios.fruitSpawnAudio);
		source->Play();
	}

	ParticleSystem* spark = bomb->AddComponent<ParticleSystem>(50, sparkModifier);
	spark->SetParticleLifeTime(1, 1);
	spark->texture = game->textures.sparkTexture;
	spark->spawnAmount = 1;
	spark->spawnFrequency = 10;
	spark->relativeOffset = glm::vec3(0, 1, 0);
	spark->maxSpawnDirectionDeviation = 15;
	spark->useGravity = false;
	spark->scale = 0.7f * glm::vec3(0.7, 1, 1);
	spark->spawnDirection = glm::vec3(0, 4, 0);

	ParticleSystem* smoke = bomb->AddComponent<ParticleSystem>(50, smokeModifier);
	smoke->SetParticleLifeTime(1, 1);
	smoke->texture = game->textures.smokeTexture;
	smoke->spawnAmount = 1;
	smoke->spawnFrequency = 20;
	smoke->relativeOffset = glm::vec3(0, 1, 0);
	smoke->maxSpawnDirectionDeviation = 0;
	smoke->useGravity = false;
	smoke->is3D = false;
	smoke->scale = 0.5f * glm::vec3(1, 1, 1);
	smoke->spawnDirection = glm::vec3(0, 1, 0);
}

static Object* createFruitParticleSystem() {
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

shared_ptr<Object> ClassicMode::createUIObject(std::shared_ptr<Model> model) const {
	auto obj = Object::Create(false);
	Renderer* renderer = obj->AddComponent<Renderer>(model);
	renderer->drawOutline = false;

	Rigidbody* rigidbody = obj->AddComponent<Rigidbody>();
	rigidbody->useGravity = false;
	glm::vec3 torque(
		randFloat(setting.spawnMinRotation.x, setting.spawnMaxRotation.x),
		randFloat(setting.spawnMinRotation.y, setting.spawnMaxRotation.y),
		randFloat(setting.spawnMinRotation.z, setting.spawnMaxRotation.z)
	);
	rigidbody->AddRelativeTorque(torque, ForceMode::Impulse);
	return obj;
}

shared_ptr<Object> ClassicMode::createUIObject(std::shared_ptr<Model> model, glm::vec4 outlineColor) const {
	auto obj = Object::Create(false);
	Renderer* renderer = obj->AddComponent<Renderer>(model);
	renderer->drawOutline = true;
	renderer->outlineColor = outlineColor;
	
	Rigidbody* rigidbody = obj->AddComponent<Rigidbody>();
	rigidbody->useGravity = false;
	glm::vec3 torque(
		randFloat(setting.spawnMinRotation.x, setting.spawnMaxRotation.x),
		randFloat(setting.spawnMinRotation.y, setting.spawnMaxRotation.y),
		randFloat(setting.spawnMinRotation.z, setting.spawnMaxRotation.z)
	);
	rigidbody->AddRelativeTorque(torque, ForceMode::Impulse);
	return obj;
}

void ClassicMode::Init() {
	state.fruitChannel.killHeight = setting.fruitKillHeight;
	state.fruitChannel.particleSystemPool = make_unique<ObjectPool<Object>>(50, createFruitParticleSystem);
	state.bombChannel.killHeight = setting.bombKillHeight;

	state.current = ClassicModeState::START;
	// BackUI
	ui.background = make_unique<UI>(game->textures.backgroundTexture);
	ui.backgroundText = make_unique<UI>(0, "Fruit Ninja", 100);
	ui.backgroundText->textColor = glm::vec4(1, 1, 0, 1);

	// Start Game Button
	FruitAsset watermelonAsset = {
		game->models.watermelonTopModel,
		game->models.watermelonBottomModel,
		game->audios.fruitSliceAudio2,
		game->audios.fruitMissAudio
	};

	ui.startGame = createUIObject(game->models.watermelonModel, glm::vec4(0, 1, 0, 1));
	auto slicable = ui.startGame->AddComponent<Fruit>(setting.sizeWatermelon, 0, setting.fruitSliceForce, state.fruitChannel, watermelonAsset);
	slicable->color = glm::vec4(1, 0, 0, 1);
	slicable->slicedParticleTexture = game->textures.sliceParticleTexture;

	// Exit button
	ui.exitGame = createUIObject(game->models.bombModel, glm::vec4(1, 0, 0, 1));
	ui.exitGame->AddComponent<Fruit>(setting.sizeBomb, 0, setting.fruitSliceForce, state.fruitChannel, FruitAsset{});

	// Reset Button
	FruitAsset pineappleAsset = {
		game->models.pineappleTopModel,
		game->models.pineappleBottomModel,
		game->audios.fruitSliceAudio1,
		game->audios.fruitMissAudio
	};

	ui.restart = createUIObject(game->models.pineappleModel);
	slicable = ui.restart->AddComponent<Fruit>(setting.sizePineapple, 0, setting.fruitSliceForce, state.fruitChannel, pineappleAsset);
	slicable->color = glm::vec4(1, 1, 0, 1);
	slicable->slicedParticleTexture = game->textures.sliceParticleTexture;

	// Audio & game->player
	game->player->AddComponent<Camera>(1.0f, 300.0f);
	game->player->transform.SetPosition(glm::vec3(0, 0, 30));
	game->player->transform.LookAt(game->player->transform.position() + glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
	game->player->AddComponent<AudioListener>();

	AudioSource* audioSource = game->player->AddComponent<AudioSource>();
	audioSource->SetAudioClip(game->audios.startMenuAudio);
	audioSource->SetLoopEnabled(true);
	audioSource->Play();
	alDistanceModel(AL_NONE);

	// Set Button position
	glm::mat4 inverse = glm::inverse(Camera::main->Perspective() * Camera::main->View());
	float z = Camera::main->ComputerNormalizedZ(30);
	glm::vec4 startPos = inverse * glm::vec4(setting.startButtonPos, z, 1);
	startPos /= startPos.w;
	ui.startGame->transform.SetPosition(startPos);

	glm::vec4 exitPos = inverse * glm::vec4(setting.exitButtonPos, z, 1);
	exitPos /= exitPos.w;
	ui.exitGame->transform.SetPosition(exitPos);

	// Front UI
	ui.fadeOutEffect = make_unique<UI>(game->textures.fadeTexture);
	ui.scoreBoard = make_unique<UI>(0, "Score: ");
	ui.scoreBoard->textColor = glm::vec4(1, 1, 0, 1);

	ui.largeScoreBoard = make_unique<UI>(0, "Score", 70);
	ui.largeScoreBoard->textColor = glm::vec4(1, 1, 0, 1);

	ui.startButton = make_unique<UI>(0, "Start");
	ui.startButton->textColor = glm::vec4(0, 1, 0, 1);

	ui.exitButton = make_unique<UI>(0, "Exit");
	ui.exitButton->textColor = glm::vec4(1, 0, 0, 1);

	ui.restartButton = make_unique<UI>(0, "Restart");
	ui.restartButton->textColor = glm::vec4(1, 1, 0, 1);
	ui.restartButton->transform.SetPosition(setting.finalScorePos);

	for (auto i = 0; i < 3; i++) {
		ui.redCrosses[i] = make_unique<UI>(game->textures.redCross);
		ui.emptyCrosses[i] = make_unique<UI>(game->textures.emptyCross);
		glm::mat4 rotation = glm::rotate(glm::mat4(1), (float)glm::radians(10 + 10.0f * (2 - i)), glm::vec3(0, 0, -1));
		ui.redCrosses[i]->transform.SetRotation(rotation);
		ui.redCrosses[i]->transform.SetScale(glm::vec3(0.1, 0.1, 0.1));
		ui.emptyCrosses[i]->transform.SetRotation(rotation);
		ui.emptyCrosses[i]->transform.SetScale(glm::vec3(0.05, 0.05, 0.05));
	}

	// VFX
	vfx.rayColor = glm::vec4(0.94, 0.94, 0.90, 1);
}

void ClassicMode::OnEnter(){
	state.current = ClassicModeState::START;
	ui.startGame->SetEnable(true);
	ui.exitGame->SetEnable(true);
}

void ClassicMode::DrawVFX(Shader& vfxShader) {
	if (state.current == ClassicModeState::EXPLOSION) {
		vfx.transform.SetPosition(state.bombChannel.explosionPosition);
		vfx.rayLength = 500 * state.explosionTimer / setting.explosionDuration;
		vfx.rayOpacity = 2 * state.explosionTimer / setting.explosionDuration;
		vfx.Draw(vfxShader);
	}
}

void ClassicMode::DrawBackUI(Shader& uiuiShader) {
	ui.background->Draw(uiuiShader);

	if (state.current == ClassicModeState::START) {
		ui.backgroundText->Draw(uiuiShader);
	}
}

void ClassicMode::DrawFrontUI(Shader& uiShader) {
	if (state.current == ClassicModeState::EXPLOSION) {
		game->enableMouseTrail = false;
		state.drawFadeOut = true;
		if (state.explosionTimer > setting.fadeStart) {
			ui.fadeOutEffect->imageColor.a = 1;
			ui.fadeOutEffect->transform.SetScale((50 * (state.explosionTimer - setting.fadeStart) / setting.explosionDuration + 1e-6f) * glm::vec3(1, 1, 1));
			ui.fadeOutEffect->Draw(uiShader);
		}
		return;
	};
	game->enableMouseTrail = true;

	if (state.drawFadeOut && state.current == ClassicModeState::SCORE && ui.fadeOutEffect->imageColor.a > 0) {
		ui.fadeOutEffect->imageColor.a -= Time::deltaTime();
		if (ui.fadeOutEffect->imageColor.a < 0) {
			ui.fadeOutEffect->imageColor.a = 0;
			state.drawFadeOut = false;
		}
		ui.fadeOutEffect->Draw(uiShader);
	}

	float halfWidth = SCR_WIDTH / 2.0f;
	float halfHeight = SCR_HEIGHT / 2.0f;
	if (state.current == ClassicModeState::State::START) {
		ui.startButton->DrawInNDC(setting.startButtonPos, uiShader);
		ui.exitButton->DrawInNDC(setting.exitButtonPos, uiShader);
	}
	else if (state.current == ClassicModeState::GAME) {
		uiShader.SetVec4("textColor", setting.scoreColor);
		ui.scoreBoard->transform.SetPosition(glm::vec3(-0.87 * halfWidth, 0.9 * halfHeight, 0));
		ui.scoreBoard->UpdateText("Score: " + to_string(state.fruitChannel.score));
		ui.scoreBoard->Draw(uiShader);

		for (int i = 0; i < 3; i++) {
			if (i < state.fruitChannel.miss) {
				ui.redCrosses[i]->transform.SetPosition(glm::vec3((0.95 - i * 0.06) * halfWidth, (0.8 + (i + 1) * 0.04) * halfHeight, 0));
				ui.redCrosses[i]->Draw(uiShader);
			}
			else {
				ui.emptyCrosses[i]->transform.SetPosition(glm::vec3((0.95 - i * 0.06) * halfWidth, (0.8 + (i + 1) * 0.04) * halfHeight, 0));
				ui.emptyCrosses[i]->Draw(uiShader);
			}
		}
	}
	else {
		uiShader.SetVec4("textColor", setting.restartColor);
		ui.restartButton->DrawInNDC(setting.startButtonPos, uiShader);

		uiShader.SetVec4("textColor", setting.exitColor);
		ui.exitButton->DrawInNDC(setting.exitButtonPos, uiShader);

		uiShader.SetVec4("textColor", setting.scoreColor);
		ui.largeScoreBoard->UpdateText("Score: " + to_string(state.fruitChannel.score));
		ui.largeScoreBoard->Draw(uiShader);
	}
}

void ClassicMode::OnExit() {}

void ClassicMode::enterGame() {
	state.current = ClassicModeState::State::GAME;
	ui.exitGame->GetComponent<Rigidbody>()->useGravity = true;
	state.fruitChannel.score = 0;
	state.spawnTimer = 0;
	state.fruitChannel.miss = 0;
	state.fruitChannel.recovery = 0;
	state.bombChannel.bombHit = false;
	game->player->GetComponent<AudioSource>()->Pause();
	auto source = acquireAudioSource();
	if (source) {
		auto audioSource = source->GetComponent<AudioSource>();
		audioSource->SetAudioClip(game->audios.gameStartAudio);
		audioSource->Play();
	}
}

void ClassicMode::enterExplosion() {
	state.current = ClassicModeState::ClassicModeState::ClassicModeState::State::EXPLOSION;
	Time::timeScale = 0;
	state.explosionTimer = 0;
	state.fruitChannel.enableSlicing = false;
	state.bombChannel.enableSlicing = false;
	
}

void ClassicMode::enterScore() {
	state.current = ClassicModeState::State::SCORE;
	ui.exitGame->SetEnable(true);
	auto rb = ui.exitGame->GetComponent<Rigidbody>();
	rb->useGravity = false;
	rb->velocity = glm::vec3(0);
	ui.restart->SetEnable(true);

	glm::mat4 inverse = glm::inverse(Camera::main->Perspective() * Camera::main->View());
	float z = Camera::main->ComputerNormalizedZ(30);
	glm::vec4 startPos = inverse * glm::vec4(setting.startButtonPos, z, 1);
	startPos /= startPos.w;
	ui.restart->transform.SetPosition(startPos);

	glm::vec4 exitPos = inverse * glm::vec4(setting.exitButtonPos, z, 1);
	exitPos /= exitPos.w;
	ui.exitGame->transform.SetPosition(exitPos);

	auto source = acquireAudioSource();
	if (source) {
		auto audioSource = source->GetComponent<AudioSource>();
		audioSource->SetAudioClip(game->audios.gameOverAudio);
		audioSource->Play();
	}

	state.fruitChannel.disableNonUI = true;
	state.bombChannel.disableAll = true;
}

void ClassicMode::exitExplosion() {
	state.fruitChannel.enableSlicing = true;
	state.bombChannel.enableSlicing = true;
}

void ClassicMode::exitScore() {
	state.fruitChannel.disableNonUI = false;
	state.bombChannel.disableAll = false;
}

void ClassicMode::processStart() {
	if (!ui.startGame->IsActive()) {
		enterGame();
	}
	else if (!ui.exitGame->IsActive()) {
		glfwSetWindowShouldClose(window, true);
	}
}

void ClassicMode::processGame() {
	state.spawnTimer += Time::deltaTime();
	if (state.spawnTimer >= state.spawnCooldown) {
		int spawnAmount = round(randFloat(setting.spawnAmountMin, setting.spawnAmountMax));
		for (int i = 0; i < spawnAmount; i++) {
			int index = round(randFloat(0, fruitSpawners.size() - 1));
			(this->fruitSpawners[index])();
		}
		state.spawnCooldown = round(randFloat(setting.spawnCooldownMin, setting.spawnCooldownMax));
		state.spawnTimer = 0;

		auto audioSourceObj = acquireAudioSource();
		if (audioSourceObj) {
			auto source = audioSourceObj->GetComponent<AudioSource>();
			source->SetAudioClip(game->audios.fruitSpawnAudio);
			source->Play();
		}
	}
	if (state.bombChannel.bombHit) {
		enterExplosion();
	}
	else if (state.fruitChannel.miss >= setting.missTolerence) {
		enterScore();
	}

	if (state.fruitChannel.recentlyRecovered) {
		state.fruitChannel.recentlyRecovered = false;
		auto source = acquireAudioSource();
		if (source) {
			auto audioSource = source->GetComponent<AudioSource>();
			audioSource->SetAudioClip(game->audios.recoveryAudio);
			audioSource->Play();
		}
	}
}

void ClassicMode::processExplosion() {
	state.explosionTimer += Time::unscaledDeltaTime();
	if (state.explosionTimer > setting.explosionDuration) {
		Time::timeScale = 1;
		exitExplosion();
		enterScore();
	}
}

void ClassicMode::processScore() {
	if (!ui.restart->IsActive()) {
		exitScore();
		enterGame();
	}
	else if (!ui.exitGame->IsActive()) {
		glfwSetWindowShouldClose(window, true);
	}
}

optional<type_index> ClassicMode::Step() {
	switch (state.current) {
		case ClassicModeState::State::START:
				processStart();
				break;
		case ClassicModeState::State::GAME:
				processGame();
				break;
		case ClassicModeState::State::EXPLOSION:
			processExplosion();
			break;
		case ClassicModeState::State::SCORE:
			processScore();
			break;
	}
	return {};
}
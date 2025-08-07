#include "classic.hpp"
#include "physics/rigidbody.hpp"
#include "audio/audiosource.hpp"
#include "audio/audiolistener.hpp"
#include "audio/audiosource_pool.hpp"
#include "rendering/renderer.hpp"
#include "rendering/particle_system.hpp"
#include "rendering/camera.hpp"

using namespace std;

static float randFloat(float min, float max) {
	return rand() / static_cast<float>(RAND_MAX) * (max - min) + min;
}

ClassicMode::ClassicMode(Game& game) 
	: GameState(game), 
	fruitSpawners {
		bind(&ClassicMode::spawnCoconut, this), bind(&ClassicMode::spawnApple, this), bind(&ClassicMode::spawnPineapple, this), bind(&ClassicMode::spawnWatermelon, this), bind(&ClassicMode::spawnBomb, this)
	}
{
}

void ClassicMode::OnFruitHit(Transform& fruit, int score) {
	context.score += score;
}

void ClassicMode::OnFruitMissed() {
	context.miss += 1;
}

void ClassicMode::OnBombHit(Transform& bomb) {
	if (context.bombHit) return;
	context.bombHit = true;
	context.explosionPosition = bomb.position();
}

void ClassicMode::spawnFruit(shared_ptr<Model>& fruitModel, shared_ptr<Model>& slice1Model, shared_ptr<Model>& slice2Model, shared_ptr<AudioClip> sliceAudio, glm::vec4 color, float radius, int score) {
	shared_ptr<Object> fruit = game.manager.CreateObject();
	SlicableAsset asset = {
		slice1Model,
		slice2Model,
		sliceAudio,
		game.audios.fruitMissAudio
	};

	auto renderer = fruit->AddComponent<Renderer>(fruitModel);
	renderer->drawOverlay = true;
	Slicable* ft = fruit->AddComponent<Slicable>(radius, setting.fruitSliceForce, context.fruitChannel, asset);
	ft->onSliced = std::bind(&ClassicMode::OnFruitHit, this, std::placeholders::_1, score);
	ft->onMissed = std::bind(&ClassicMode::OnFruitMissed, this);
	ft->particleTexture = game.textures.sliceParticleTexture;
	ft->particleColor = color;
	Rigidbody* rb = fruit->AddComponent<Rigidbody>();
	float upForce = randFloat(setting.fruitUpMin, setting.fruitUpMax);
	float horizontalForce = randFloat(setting.fruitHorizontalMin, setting.fruitHorizontalMax);
	rb->AddForce(game.gameClock, glm::vec3(horizontalForce, upForce, 0), ForceMode::Impulse);

	glm::vec3 torque(
		randFloat(setting.spawnMinRotation.x, setting.spawnMaxRotation.x),
		randFloat(setting.spawnMinRotation.y, setting.spawnMaxRotation.y),
		randFloat(setting.spawnMinRotation.z, setting.spawnMaxRotation.z)
	);

	rb->AddRelativeTorque(game.gameClock, torque, ForceMode::Impulse);
	float startX = randFloat(setting.fruitSpawnCenter - setting.fruitSpawnWidth / 2, setting.fruitSpawnCenter + setting.fruitSpawnWidth / 2);
	glm::vec3 position(startX, setting.fruitSpawnHeight, 0);
	// cout << "Spawn at " << glm::to_string(position) << "\n";
	rb->transform.SetPosition(position);
}

void ClassicMode::spawnApple() {
	spawnFruit(game.models.appleModel, game.models.appleTopModel, game.models.appleBottomModel, game.audios.fruitSliceAudio1, glm::vec4(1, 1, 1, 1), setting.sizeApple);
}

void ClassicMode::spawnPineapple() {
	spawnFruit(game.models.pineappleModel, game.models.pineappleTopModel, game.models.pineappleBottomModel, game.audios.fruitSliceAudio1, glm::vec4(1, 1, 0, 1), setting.sizePineapple);
}

void ClassicMode::spawnWatermelon() {
	spawnFruit(game.models.watermelonModel, game.models.watermelonTopModel, game.models.watermelonBottomModel, game.audios.fruitSliceAudio2, glm::vec4(1, 0, 0, 1), setting.sizeWatermelon);
}

void ClassicMode::spawnCoconut() {
	spawnFruit(game.models.coconutModel, game.models.coconutTopModel, game.models.coconutBottomModel, game.audios.fruitSliceAudio2, glm::vec4(1, 1, 1, 1), setting.sizeCoconut);
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
	shared_ptr<Object> bomb = game.manager.CreateObject();
	bomb->transform.SetScale(0.7f * glm::vec3(1, 1, 1));

	auto renderer = bomb->AddComponent<Renderer>(game.models.bombModel);
	renderer->drawOverlay = true;
	renderer->drawOutline = true;
	renderer->outlineColor = glm::vec4(1, 0, 0, 1);
	auto slicable = bomb->AddComponent<Slicable>(setting.sizeBomb, 0, context.bombChannel, SlicableAsset{ .clipOnSliced = game.audios.explosionAudio });
	slicable->onSliced = std::bind(&ClassicMode::OnBombHit, this, std::placeholders::_1);
	slicable->destroyOnSliced = false;

	AudioSource* audioSource = bomb->AddComponent<AudioSource>();
	audioSource->SetAudioClip(game.audios.fuseAudio);
	audioSource->SetLoopEnabled(true);
	audioSource->Play();

	Rigidbody* rb = bomb->AddComponent<Rigidbody>();
	float upForce = randFloat(setting.fruitUpMin, setting.fruitUpMax);
	float horizontalForce = randFloat(setting.fruitHorizontalMin, setting.fruitHorizontalMax);
	rb->AddForce(game.gameClock, glm::vec3(horizontalForce, upForce, 0), ForceMode::Impulse);

	glm::vec3 torque(
		randFloat(setting.spawnMinRotation.x, setting.spawnMaxRotation.x),
		randFloat(setting.spawnMinRotation.y, setting.spawnMaxRotation.y),
		randFloat(setting.spawnMinRotation.z, setting.spawnMaxRotation.z)
	);

	rb->AddRelativeTorque(game.gameClock, torque, ForceMode::Impulse);
	float startX = randFloat(setting.fruitSpawnCenter - setting.fruitSpawnWidth / 2, setting.fruitSpawnCenter + setting.fruitSpawnWidth / 2);
	glm::vec3 position(startX, setting.fruitSpawnHeight, 5);
	// cout << "Spawn at " << glm::to_string(position) << "\n";
	rb->transform.SetPosition(position);

	auto audioSourceObj = acquireAudioSource();
	if (audioSourceObj) {
		auto source = audioSourceObj->GetComponent<AudioSource>();
		source->SetAudioClip(game.audios.fruitSpawnAudio);
		source->Play();
	}

	ParticleSystem* spark = bomb->AddComponent<ParticleSystem>(50, sparkModifier);
	spark->SetParticleLifeTime(1, 1);
	spark->texture = game.textures.sparkTexture;
	spark->spawnAmount = 1;
	spark->spawnFrequency = 10;
	spark->relativeOffset = glm::vec3(0, 1, 0);
	spark->maxSpawnDirectionDeviation = 15;
	spark->useGravity = false;
	spark->scale = 0.7f * glm::vec3(0.7, 1, 1);
	spark->spawnDirection = glm::vec3(0, 4, 0);

	ParticleSystem* smoke = bomb->AddComponent<ParticleSystem>(50, smokeModifier);
	smoke->SetParticleLifeTime(1, 1);
	smoke->texture = game.textures.smokeTexture;
	smoke->spawnAmount = 1;
	smoke->spawnFrequency = 20;
	smoke->relativeOffset = glm::vec3(0, 1, 0);
	smoke->maxSpawnDirectionDeviation = 0;
	smoke->useGravity = false;
	smoke->is3D = false;
	smoke->scale = 0.5f * glm::vec3(1, 1, 1);
	smoke->spawnDirection = glm::vec3(0, 1, 0);
}

void ClassicMode::Init() {
	context.fruitChannel = std::make_shared<SlicableControl>();
	context.fruitChannel->killHeight = setting.fruitKillHeight;
	context.fruitChannel->particlePool = make_unique<ObjectPool<Object>>(50, std::bind(&Game::createFruitParticleSystem, &game));

	context.bombChannel = std::make_shared<SlicableControl>();
	context.bombChannel->killHeight = setting.bombKillHeight;

	context.current = ClassicModeContext::Start;
	// BackUI
	ui.background = make_unique<UI>(game.textures.backgroundTexture);
	ui.backgroundText = make_unique<UI>(0, "Fruit Ninja", 100);
	ui.backgroundText->textColor = glm::vec4(1, 1, 0, 1);

	// Start Game Button
	SlicableAsset watermelonAsset = {
		game.models.watermelonTopModel,
		game.models.watermelonBottomModel,
		game.audios.fruitSliceAudio2,
	};

	ui.startGame = game.createUIObject(game.models.watermelonModel, glm::vec4(0, 1, 0, 1));
	auto slicable = ui.startGame->AddComponent<Slicable>(setting.sizeWatermelon, setting.fruitSliceForce, game.uiConfig.control, watermelonAsset);
	slicable->particleColor = glm::vec4(1, 0, 0, 1);
	slicable->particleTexture = game.textures.sliceParticleTexture;

	// Exit button
	ui.back = game.createUIObject(game.models.bombModel, glm::vec4(1, 0, 0, 1));
	ui.back->AddComponent<Slicable>(setting.sizeBomb, setting.fruitSliceForce, game.uiConfig.control, SlicableAsset{});

	// Reset Button
	SlicableAsset pineappleAsset = {
		game.models.pineappleTopModel,
		game.models.pineappleBottomModel,
		game.audios.fruitSliceAudio1,
	};

	ui.restart = game.createUIObject(game.models.pineappleModel);
	slicable = ui.restart->AddComponent<Slicable>(setting.sizePineapple, setting.fruitSliceForce, game.uiConfig.control, pineappleAsset);
	slicable->particleColor = glm::vec4(1, 1, 0, 1);
	slicable->particleTexture = game.textures.sliceParticleTexture;

	// Front UI
	ui.fadeOutEffect = make_unique<UI>(game.textures.fadeTexture);
	ui.scoreBoard = make_unique<UI>(0, "Score: ");
	ui.scoreBoard->textColor = glm::vec4(1, 1, 0, 1);

	ui.largeScoreBoard = make_unique<UI>(0, "Score", 70);
	ui.largeScoreBoard->textColor = glm::vec4(1, 1, 0, 1);

	ui.startButton = make_unique<UI>(0, "Start");
	ui.startButton->textColor = glm::vec4(0, 1, 0, 1);

	ui.backButton = make_unique<UI>(0, "Back");
	ui.backButton->textColor = glm::vec4(1, 0, 0, 1);

	ui.restartButton = make_unique<UI>(0, "Restart");
	ui.restartButton->textColor = glm::vec4(1, 1, 0, 1);
	ui.restartButton->transform.SetPosition(setting.finalScorePos);

	for (auto i = 0; i < 3; i++) {
		ui.redCrosses[i] = make_unique<UI>(game.textures.redCross);
		ui.emptyCrosses[i] = make_unique<UI>(game.textures.emptyCross);
		glm::mat4 rotation = glm::rotate(glm::mat4(1), (float)glm::radians(10 + 10.0f * (2 - i)), glm::vec3(0, 0, -1));
		ui.redCrosses[i]->transform.SetRotation(rotation);
		ui.redCrosses[i]->transform.SetScale(glm::vec3(0.1, 0.1, 0.1));
		ui.emptyCrosses[i]->transform.SetRotation(rotation);
		ui.emptyCrosses[i]->transform.SetScale(glm::vec3(0.05, 0.05, 0.05));
	}

	// VFX
	vfx.rayColor = glm::vec4(0.94, 0.94, 0.90, 1);
}

void ClassicMode::OnDrawVFX(Shader& vfxShader) {
	if (context.current == ClassicModeContext::Explosion) {
		vfx.transform.SetPosition(context.explosionPosition);
		vfx.rayLength = 500 * context.explosionTimer / setting.explosionDuration;
		vfx.rayOpacity = 2 * context.explosionTimer / setting.explosionDuration;
		vfx.Draw(vfxShader);
	}
}

void ClassicMode::OnDrawBackUI(Shader& uiShader) {
	if (context.current == ClassicModeContext::Start) {
		ui.backgroundText->Draw(uiShader);
	}
}

void ClassicMode::OnDrawFrontUI(Shader& uiShader) {
	auto screenSize = RenderContext::Context->Dimension();
	if (context.current == ClassicModeContext::Explosion) {
		game.enableMouseTrail = false;
		context.drawFadeOut = true;
		if (context.explosionTimer > setting.fadeStart) {
			ui.fadeOutEffect->imageColor.a = 1;
			ui.fadeOutEffect->transform.SetScale((50 * (context.explosionTimer - setting.fadeStart) / setting.explosionDuration + 1e-6f) * glm::vec3(1, 1, 1));
			ui.fadeOutEffect->Draw(uiShader);
		}
		return;
	};
	game.enableMouseTrail = true;

	if (context.drawFadeOut && context.current == ClassicModeContext::Score && ui.fadeOutEffect->imageColor.a > 0) {
		ui.fadeOutEffect->imageColor.a -= game.gameClock.DeltaTime();
		if (ui.fadeOutEffect->imageColor.a < 0) {
			ui.fadeOutEffect->imageColor.a = 0;
			context.drawFadeOut = false;
		}
		ui.fadeOutEffect->Draw(uiShader);
	}

	float halfWidth = screenSize.x / 2.0f;
	float halfHeight = screenSize.y / 2.0f;
	if (context.current == ClassicModeContext::Start) {
		ui.startButton->DrawInNDC(setting.startButtonPos, uiShader);
		ui.backButton->DrawInNDC(setting.exitButtonPos, uiShader);
	}
	else if (context.current == ClassicModeContext::Game) {
		uiShader.SetVec4("textColor", setting.scoreColor);
		ui.scoreBoard->transform.SetPosition(glm::vec3(-0.87 * halfWidth, 0.9 * halfHeight, 0));
		ui.scoreBoard->UpdateText("Score: " + to_string(context.score));
		ui.scoreBoard->Draw(uiShader);

		for (int i = 0; i < 3; i++) {
			if (i < context.miss) {
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
		ui.backButton->DrawInNDC(setting.exitButtonPos, uiShader);

		uiShader.SetVec4("textColor", setting.scoreColor);
		ui.largeScoreBoard->UpdateText("Score: " + to_string(context.score));
		ui.largeScoreBoard->Draw(uiShader);
	}
}

void ClassicMode::OnEnter() {
	context.current = ClassicModeContext::Start;
	game.manager.Register(ui.startGame);
	game.manager.Register(ui.back);
	PositionUI();

	Rigidbody* rb = ui.startGame->GetComponent<Rigidbody>();
	rb->velocity = {};
	rb->useGravity = false;

	rb = ui.restart->GetComponent<Rigidbody>();
	rb->velocity = {};
	rb->useGravity = false;

	rb = ui.back->GetComponent<Rigidbody>();
	rb->velocity = {};
	rb->useGravity = false;

	game.uiConfig.control->disableSlicing = true;
	StartCoroutine(FadeInUI(1.0f));
}

void ClassicMode::OnExit() {
	ui.startGame->GetComponent<Rigidbody>()->useGravity = true;
	ui.restart->GetComponent<Rigidbody>()->useGravity = true;
	ui.back->GetComponent<Rigidbody>()->useGravity = true;
	StartCoroutine(FadeOutUI(1.0f));
}

void ClassicMode::enterGame() {
	context.current = ClassicModeContext::Game;
	context.fruitChannel->disableAll = false;
	context.bombChannel->disableAll = false;

	ui.back->GetComponent<Rigidbody>()->useGravity = true;
	context.score = 0;
	context.spawnTimer = 0;
	context.miss = 0;
	context.recovery = 0;
	context.bombHit = false;
	game.player->GetComponent<AudioSource>()->Pause();
	auto source = acquireAudioSource();
	if (source) {
		auto audioSource = source->GetComponent<AudioSource>();
		audioSource->SetAudioClip(game.audios.gameStartAudio);
		audioSource->Play();
	}
	StartCoroutine(FadeOutUI(0.5f));
}

void ClassicMode::enterExplosion() {
	context.current = ClassicModeContext::Explosion;
	game.gameClock.timeScale = 0;
	context.explosionTimer = 0;
	context.fruitChannel->disableSlicing = true;
	context.bombChannel->disableSlicing = true;
}

void ClassicMode::enterScore() {
	context.current = ClassicModeContext::Score;

	PositionUI();
	game.manager.Register(ui.back);
	auto rb = ui.back->GetComponent<Rigidbody>();
	rb->useGravity = false;
	rb->velocity = glm::vec3(0);

	game.manager.Register(ui.restart);
	rb = ui.restart->GetComponent<Rigidbody>();
	rb->velocity = {};
	rb->useGravity = false;

	StartCoroutine(FadeInUI(0.7f));

	auto source = acquireAudioSource();
	if (source) {
		auto audioSource = source->GetComponent<AudioSource>();
		audioSource->SetAudioClip(game.audios.gameOverAudio);
		audioSource->Play();
	}

	context.fruitChannel->disableAll = true;
	context.bombChannel->disableAll = true;
}

void ClassicMode::exitExplosion() {
	context.fruitChannel->disableSlicing = false;
	context.bombChannel->disableSlicing = false;
}

void ClassicMode::exitScore() {
	StartCoroutine(FadeOutUI(0.7f));
}

void ClassicMode::processStart() {
	
}

void ClassicMode::processGame() {
	context.spawnTimer += game.gameClock.DeltaTime();
	if (context.spawnTimer >= context.spawnCooldown) {
		int spawnAmount = round(randFloat(setting.spawnAmountMin, setting.spawnAmountMax));
		for (int i = 0; i < spawnAmount; i++) {
			int index = round(randFloat(0, fruitSpawners.size() - 1));
			(this->fruitSpawners[index])();
		}
		context.spawnCooldown = round(randFloat(setting.spawnCooldownMin, setting.spawnCooldownMax));
		context.spawnTimer = 0;

		auto audioSourceObj = acquireAudioSource();
		if (audioSourceObj) {
			auto source = audioSourceObj->GetComponent<AudioSource>();
			source->SetAudioClip(game.audios.fruitSpawnAudio);
			source->Play();
		}
	}
	if (context.bombHit) {
		enterExplosion();
	}
	else if (context.miss >= setting.missTolerence) {
		enterScore();
	}

	if (context.recentlyRecovered) {
		context.recentlyRecovered = false;
		auto source = acquireAudioSource();
		if (source) {
			auto audioSource = source->GetComponent<AudioSource>();
			audioSource->SetAudioClip(game.audios.recoveryAudio);
			audioSource->Play();
		}
	}
}

void ClassicMode::processExplosion() {
	context.explosionTimer += game.gameClock.UnscaledDeltaTime();
	if (context.explosionTimer > setting.explosionDuration) {
		game.gameClock.timeScale = 1;
		exitExplosion();
		enterScore();
	}
}

optional<type_index> ClassicMode::Step() {
	PositionUI();
	switch (context.current) {
		case ClassicModeContext::Start:
			if (!ui.startGame->IsActive()) {
				enterGame();
			}
			else if (!ui.back->IsActive()) { // Exit game
				return {}; 
			};
			break;
		case ClassicModeContext::Game:
			processGame();
			break;
		case ClassicModeContext::Explosion:
			processExplosion();
			break;
		case ClassicModeContext::Score:
			if (!ui.restart->IsActive()) {
				exitScore();
				enterGame();
			}
			else if (!ui.back->IsActive()) {
				return {};
			}
			break;
	}
	return Self();
}

bool ClassicMode::AllowParentFrontUI() { return false; }

void ClassicMode::PositionUI() {
	glm::mat4 inverse = glm::inverse(Camera::main->Perspective() * Camera::main->View());
	float z = Camera::main->ComputerNormalizedZ(30);

	if (context.current == ClassicModeContext::Start) {
		glm::vec4 startPos = inverse * glm::vec4(setting.startButtonPos, z, 1);
		startPos /= startPos.w;
		ui.startGame->transform.SetPosition(startPos);

		glm::vec4 exitPos = inverse * glm::vec4(setting.exitButtonPos, z, 1);
		exitPos /= exitPos.w;
		ui.back->transform.SetPosition(exitPos);
	} else if(context.current == ClassicModeContext::Score) {
		glm::vec4 startPos = inverse * glm::vec4(setting.startButtonPos, z, 1);
		startPos /= startPos.w;
		ui.restart->transform.SetPosition(startPos);

		glm::vec4 exitPos = inverse * glm::vec4(setting.exitButtonPos, z, 1);
		exitPos /= exitPos.w;
		ui.back->transform.SetPosition(exitPos);
	}
}

Coroutine ClassicMode::FadeInUI(float duration) {
	float time = 0;

	Renderer* back = ui.back->GetComponent<Renderer>();
	Renderer* start = ui.startGame->GetComponent<Renderer>();
	Renderer* restart = ui.restart->GetComponent<Renderer>();
	
	while (time < duration) {
		time += game.gameClock.DeltaTime();
		float opacity = time / duration;
		back->color.a = opacity;
		back->outlineColor.a = opacity;
		start->color.a = opacity;
		start->outlineColor.a = opacity;
		restart->color.a = opacity;
		restart->outlineColor.a = opacity;

		ui.backButton->textColor.a = opacity;
		ui.startButton->textColor.a = opacity;
		ui.restartButton->textColor.a = opacity;
		co_yield {};
	}
	back->color.a = 1;
	start->color.a = 1;
	restart->color.a = 1;
	back->outlineColor.a = 1;
	start->outlineColor.a = 1;
	restart->outlineColor.a = 1;

	ui.backButton->textColor.a = 1;
	ui.startButton->textColor.a = 1;
	ui.restartButton->textColor.a = 1;
	game.uiConfig.control->disableSlicing = false;
}

Coroutine ClassicMode::FadeOutUI(float duration) {
	float time = 0;

	while (time < duration) {
		time += game.gameClock.DeltaTime();
		float opacity = 1 - time / duration;

		ui.backButton->textColor.a = opacity;
		ui.startButton->textColor.a = opacity;
		ui.restartButton->textColor.a = opacity;
		co_yield{};
	}

	ui.backButton->textColor.a = 0;
	ui.startButton->textColor.a = 0;
	ui.restartButton->textColor.a = 0;
	game.uiConfig.control->disableSlicing = false;
}
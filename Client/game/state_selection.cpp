#include "audio/audiolistener.hpp"
#include "rendering/camera.hpp"
#include "state_selection.hpp"
#include "game/classic_mode/classic.hpp"
#include "game/mtp_mode/mtp_classic.hpp"
#include "physics/rigidbody.hpp"

using namespace std;

SelectionState::SelectionState(Game& game) : GameState(game) {}

void SelectionState::Init() {
	// Camera
	auto camera = game.player->AddComponent<Camera>(1.0f, 300.0f);
	game.player->transform.SetPosition(glm::vec3(0, 0, 30));
	game.player->transform.LookAt(game.player->transform.position() + glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
	game.player->AddComponent<AudioListener>();

	// UI
	ui.backgroundImage = make_unique<UI>(game.textures.backgroundTexture);
	ui.classicModeText = make_unique<UI>(0, "Classic");
	ui.classicModeText->textColor = { 0, 1, 0, 1 };
	ui.multiplayerText = make_unique<UI>(0, "Multiplayer");
	ui.multiplayerText->textColor = { 1, 1, 0, 1 };
	ui.exitText = make_unique<UI>(0, "Exit");
	ui.exitText->textColor = { 1, 0, 0, 1 };
	ui.titleText = make_unique<UI>(0, "Fruit Ninja", 100);
	ui.titleText->textColor = { 1, 0.647, 0, 1 };
	ui.classicModeSelection = game.createUIObject(game.models.watermelonModel, glm::vec4(0, 1, 0, 1));
	ui.exitSelection = game.createUIObject(game.models.bombModel, glm::vec4(1, 0, 0, 1));
	ui.multiplayerSelection = game.createUIObject(game.models.pineappleModel, glm::vec4(1, 1, 0, 1));

	FruitAsset watermelonAsset = {
		game.models.watermelonTopModel,
		game.models.watermelonBottomModel,
		game.audios.fruitSliceAudio2,
		game.audios.fruitMissAudio
	};

	FruitAsset pineappleAsset = {
		game.models.pineappleTopModel,
		game.models.pineappleBottomModel,
		game.audios.fruitSliceAudio1,
		game.audios.fruitMissAudio
	};

	auto slicable = ui.classicModeSelection->AddComponent<Fruit>(game.uiConfig.sizeWatermelon, 0, game.uiConfig.fruitSliceForce, game.uiConfig.control, watermelonAsset);
	slicable->color = glm::vec4(1, 0, 0, 1);
	slicable->slicedParticleTexture = game.textures.sliceParticleTexture;

	slicable = ui.multiplayerSelection->AddComponent<Fruit>(game.uiConfig.sizePineapple, 0, game.uiConfig.fruitSliceForce, game.uiConfig.control, pineappleAsset);
	slicable->color = glm::vec4(1, 1, 0, 1);
	slicable->slicedParticleTexture = game.textures.sliceParticleTexture;

	ui.exitSelection->AddComponent<Fruit>(game.uiConfig.sizeBomb, 0, 0.0f, game.uiConfig.control, FruitAsset{});

	// Game Modes
	AddSubState<ClassicMode>(game);
	AddSubState<MTP_ClassicMode>(game);
}

void SelectionState::OnEnter() {
	PositionUI();
	game.manager.Register(ui.classicModeSelection);
	game.manager.Register(ui.exitSelection);
	game.manager.Register(ui.multiplayerSelection);

	Rigidbody* rb = ui.classicModeSelection->GetComponent<Rigidbody>();
	rb->velocity = {};
	rb->useGravity = false;

	rb = ui.exitSelection->GetComponent<Rigidbody>();
	rb->velocity = {};
	rb->useGravity = false;

	rb = ui.multiplayerSelection->GetComponent<Rigidbody>();
	rb->velocity = {};
	rb->useGravity = false;

	music = game.player->AddComponent<AudioSource>();
	music->SetAudioClip(game.audios.startMenuAudio);
	music->SetLoopEnabled(true);
	music->Play();
	alDistanceModel(AL_NONE);

	StartCoroutine(FadeInUI(1));
}

void SelectionState::OnExit() {
	ui.classicModeSelection->GetComponent<Rigidbody>()->useGravity = true;
	ui.exitSelection->GetComponent<Rigidbody>()->useGravity = true;
	ui.multiplayerSelection->GetComponent<Rigidbody>()->useGravity = true;
	music->Pause();
}

void SelectionState::OnEnterSubState() {
	ui.classicModeSelection->GetComponent<Rigidbody>()->useGravity = true;
	ui.exitSelection->GetComponent<Rigidbody>()->useGravity = true;
	ui.multiplayerSelection->GetComponent<Rigidbody>()->useGravity = true;
	music->Pause();
	StartCoroutine(FadeOutUI(1.0f));
}
void SelectionState::OnExitSubState() {
	PositionUI();
	game.manager.Register(ui.classicModeSelection);
	game.manager.Register(ui.exitSelection);
	game.manager.Register(ui.multiplayerSelection);
	Rigidbody* rb = ui.classicModeSelection->GetComponent<Rigidbody>();
	rb->velocity = {};
	rb->useGravity = false;
	
	rb = ui.exitSelection->GetComponent<Rigidbody>();
	rb->velocity = {};
	rb->useGravity = false;

	rb = ui.multiplayerSelection->GetComponent<Rigidbody>();
	rb->velocity = {};
	rb->useGravity = false;

	music->Play();

	game.uiConfig.control.enableSlicing = false; // The coroutine won't be called in the current frame
	StartCoroutine(FadeInUI(1.0f));
}

void SelectionState::OnDrawBackUI(Shader& uiShader) {
	ui.backgroundImage->Draw(uiShader);
}

void SelectionState::OnDrawFrontUI(Shader& uiShader) {
	ui.titleText->Draw(uiShader);
	ui.classicModeText->DrawInNDC({-0.5, -0.5}, uiShader);
	ui.multiplayerText->DrawInNDC({ 0, -0.5 }, uiShader);
	ui.exitText->DrawInNDC({ 0.5, -0.5 }, uiShader);
}

optional<type_index> SelectionState::Step() {
	PositionUI();
	if (!ui.exitSelection->IsActive()) {
		return {};
	}
	if (!ui.classicModeSelection->IsActive()) {
		return EnterSubState<ClassicMode>();
	}
	if (!ui.multiplayerSelection->IsActive()) {
		return EnterSubState<MTP_ClassicMode>();
	}
	if (!ui.multiplayerSelection->IsActive()) {
		// SetSubState<NClassicMode>();
		return Self();
	}

	// PositionUI();
	return Self();
}

void SelectionState::PositionUI() {
	// Set UI position
	glm::mat4 inverse = glm::inverse(Camera::main->Projection() * Camera::main->View());
	float z = Camera::main->ComputerNormalizedZ(30);
	glm::vec4 classicModePos = inverse * glm::vec4(-0.5, -0.5, z, 1);
	classicModePos /= classicModePos.w;
	ui.classicModeSelection->transform.SetPosition(classicModePos);

	glm::vec4 multiplayerPos = inverse * glm::vec4(0, -0.5, z, 1);
	multiplayerPos /= multiplayerPos.w;
	ui.multiplayerSelection->transform.SetPosition(multiplayerPos);

	glm::vec4 exitPos = inverse * glm::vec4(0.5, -0.5, z, 1);
	exitPos /= exitPos.w;
	ui.exitSelection->transform.SetPosition(exitPos);
}

Coroutine SelectionState::FadeInUI(float duration) {
	game.uiConfig.control.enableSlicing = false;
	float timer = 0;
	Renderer* classic = ui.classicModeSelection->GetComponent<Renderer>();
	Renderer* multiplayer = ui.multiplayerSelection->GetComponent<Renderer>();
	Renderer* exit = ui.exitSelection->GetComponent<Renderer>();
	while (timer < duration) {
		float opacity = timer / duration;
		ui.classicModeText->textColor.a = opacity;
		ui.multiplayerText->textColor.a = opacity;
		ui.exitText->textColor.a = opacity;

		PositionUI();
		classic->color.a = opacity;
		multiplayer->color.a = opacity;
		exit->color.a = opacity;

		classic->outlineColor.a = opacity;
		multiplayer->outlineColor.a = opacity;
		exit->outlineColor.a = opacity;

		timer += game.gameClock.DeltaTime();
		co_yield{};
	}
	ui.classicModeText->textColor.a = 1;
	ui.multiplayerText->textColor.a = 1;
	ui.exitText->textColor.a = 1;
	classic->color.a = 1;
	multiplayer->color.a = 1;
	exit->color.a = 1;
	classic->outlineColor.a = 1;
	multiplayer->outlineColor.a = 1;
	exit->outlineColor.a = 1;

	game.uiConfig.control.enableSlicing = true;
}
Coroutine SelectionState::FadeOutUI(float duration) {
	game.uiConfig.control.enableSlicing = false;
	float timer = 0;

	while (timer < duration) {
		float opacity = 1 - timer / duration;
		ui.classicModeText->textColor.a = opacity;
		ui.multiplayerText->textColor.a = opacity;
		ui.exitText->textColor.a = opacity;
		timer += game.gameClock.DeltaTime();
		co_yield{};
	}

	ui.classicModeText->textColor.a = 0;
	ui.multiplayerText->textColor.a = 0;
	ui.exitText->textColor.a = 0;
	game.uiConfig.control.enableSlicing = true;
}
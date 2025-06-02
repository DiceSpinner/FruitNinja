#include "classic_networking.hpp"
#include "game/modes/classic.hpp"
#include "state/time.hpp"
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

#pragma region States

#pragma region Selection
NClassicMode::Selection::Selection(Game* game, NClassicMode* mode) : GameState(game), mode(mode) { }
void NClassicMode::Selection::Init() {
	ui.guestText = make_unique<UI>(0, "Guest");
	ui.hostText = make_unique<UI>(0, "Host");
	ui.backText = make_unique<UI>(0, "Back");
	ui.guestSelection = game->createUIObject(game->models.pineappleModel, glm::vec4(1, 1, 0, 1));
	ui.hostSelection = game->createUIObject(game->models.coconutModel, glm::vec4(0.647, 0.165, 0.165, 1));
	ui.backSelection = game->createUIObject(game->models.bombModel, glm::vec4(1, 1, 0, 1));

	FruitAsset pineappleAsset = {
		game->models.pineappleTopModel,
		game->models.pineappleBottomModel,
		game->audios.fruitSliceAudio1,
		game->audios.fruitMissAudio
	};
	auto slicable = ui.guestSelection->AddComponent<Fruit>(mode->setting.sizePineapple, 0, mode->setting.fruitSliceForce, mode->context.fruitChannel, pineappleAsset);
	ui.guestSelection->AddComponent<Renderer>(game->models.pineappleModel);
	slicable->color = { 1, 1, 0, 1 };

	FruitAsset coconutAsset = {
		game->models.coconutTopModel,
		game->models.coconutBottomModel,
		game->audios.fruitSliceAudio2,
		game->audios.fruitMissAudio
	};
	slicable = ui.guestSelection->AddComponent<Fruit>(mode->setting.sizePineapple, 0, mode->setting.fruitSliceForce, mode->context.fruitChannel, pineappleAsset);
	ui.guestSelection->AddComponent<Renderer>(game->models.pineappleModel);
	slicable->color = { 1, 1, 1, 1 };

	ui.backSelection->AddComponent<Fruit>(mode->setting.sizeBomb, 0, mode->setting.fruitSliceForce, mode->context.fruitChannel, FruitAsset{});
	ui.backSelection->AddComponent<Renderer>(game->models.bombModel);
}

void NClassicMode::Selection::OnEnter() {
	ui.guestSelection->SetEnable(true);
	ui.hostSelection->SetEnable(true);
	ui.backSelection->SetEnable(true);
}

void NClassicMode::Selection::OnExit() {
	ui.guestSelection->SetEnable(false);
	ui.hostSelection->SetEnable(false);
	ui.backSelection->SetEnable(false);
}

void NClassicMode::Selection::OnDrawFrontUI(Shader& uiShader) {
	ui.hostText->DrawInNDC(glm::vec2(-0.5, -0.5), uiShader);
	ui.guestText->DrawInNDC(glm::vec2(0, -0.5), uiShader);
	ui.backText->DrawInNDC(glm::vec2(0.5, -0.5), uiShader);
}

bool NClassicMode::Selection::AllowParentBackUI() { return false;  }

optional<type_index> NClassicMode::Selection::Step() {
	if (!ui.backSelection->IsActive()) {
		return {};
	}
	if (!ui.guestSelection->IsActive()) {
		// return type_index(typeid(Guest));
	}
	if (!ui.hostSelection->IsActive()) {
		// return type_index(typeid(Host));
	}

	glm::mat4 inverse = glm::inverse(Camera::main->Perspective() * Camera::main->View());
	float z = Camera::main->ComputerNormalizedZ(30);

	glm::vec4 hostPos = inverse * glm::vec4(-0.5, -0.5, z, 1);
	hostPos /= hostPos.w;
	ui.hostSelection->transform.SetPosition(hostPos);

	glm::vec4 backPos = inverse * glm::vec4(0.5, -0.5, z, 1);
	backPos /= backPos.w;
	ui.backSelection->transform.SetPosition(backPos);

	glm::vec4 guestPos = inverse * glm::vec4(0, -0.5, z, 1);
	guestPos /= guestPos.w;
	ui.guestSelection->transform.SetPosition(guestPos);
	return type_index(typeid(this));
}

#pragma endregion

#pragma region Host



#pragma endregion

#pragma region Guest

#pragma endregion

#pragma endregion

NClassicMode::NClassicMode(Game* game)
	: GameState(game)
{
}

void NClassicMode::Init() {
	AddSubState<Selection>(game, this);

	context.fruitChannel.killHeight = setting.fruitKillHeight;
	// context.fruitChannel.particleSystemPool = make_unique<ObjectPool<Object>>(50, createFruitParticleSystem);
	context.bombChannel.killHeight = setting.bombKillHeight;
}

void NClassicMode::OnEnter() {
	EnterSubState<Selection>();
}

void NClassicMode::OnDrawVFX(Shader& vfxShader) {
}

void NClassicMode::OnDrawBackUI(Shader& uiuiShader) {
}

void NClassicMode::OnDrawFrontUI(Shader& uiShader) {

}

void NClassicMode::OnExit() {}

optional<type_index> NClassicMode::Step() {
	context.state->Run();
	return {};
}
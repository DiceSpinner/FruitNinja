#include "classic_networking.hpp"
#include "game/modes/classic.hpp"
#include "state/time.hpp"
#include "state/window.hpp"
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
	ui.guestSelection = mode->createUIObject(game->models.pineappleModel, glm::vec4(1, 1, 0, 1));
	ui.hostSelection = mode->createUIObject(game->models.coconutModel, glm::vec4(0.647, 0.165, 0.165, 1));
	ui.backSelection = mode->createUIObject(game->models.bombModel, glm::vec4(1, 1, 0, 1));

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

#pragma region Spawn Fruit

void NClassicMode::spawnFruit(shared_ptr<Model>& fruitModel, shared_ptr<Model>& slice1Model, shared_ptr<Model>& slice2Model, shared_ptr<AudioClip> sliceAudio, glm::vec4 color, float radius, int sinfrastructure) {
	shared_ptr<Object> fruit = Object::Create();
	FruitAsset asset = {
		slice1Model,
		slice2Model,
		sliceAudio,
		game->audios.fruitMissAudio
	};

	auto renderer = fruit->AddComponent<Renderer>(fruitModel);
	// renderer->drawOverlay = true;
	Fruit* ft = fruit->AddComponent<Fruit>(radius, sinfrastructure, setting.fruitSliceForce, context.fruitChannel, asset);
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

void NClassicMode::spawnApple() {
	spawnFruit(game->models.appleModel, game->models.appleTopModel, game->models.appleBottomModel, game->audios.fruitSliceAudio1, glm::vec4(1, 1, 1, 1), setting.sizeApple);
}

void NClassicMode::spawnPineapple() {
	spawnFruit(game->models.pineappleModel, game->models.pineappleTopModel, game->models.pineappleBottomModel, game->audios.fruitSliceAudio1, glm::vec4(1, 1, 0, 1), setting.sizePineapple);
}

void NClassicMode::spawnWatermelon() {
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

void NClassicMode::spawnBomb() {
	shared_ptr<Object> bomb = Object::Create();
	bomb->transform.SetScale(0.7f * glm::vec3(1, 1, 1));

	auto renderer = bomb->AddComponent<Renderer>(game->models.bombModel);
	// renderer->drawOverlay = true;
	renderer->drawOutline = true;
	renderer->outlineColor = glm::vec4(1, 0, 0, 1);
	bomb->AddComponent<Bomb>(game->audios.explosionAudio, setting.sizeBomb, context.bombChannel);

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
#pragma endregion

shared_ptr<Object> NClassicMode::createUIObject(std::shared_ptr<Model> model) const {
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

shared_ptr<Object> NClassicMode::createUIObject(std::shared_ptr<Model> model, glm::vec4 outlineColor) const {
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
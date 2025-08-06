#include "mtp_classic.hpp"
#include "multiplayer/setting.hpp"
#include "physics/rigidbody.hpp"
#include "rendering/renderer.hpp"
#include "rendering/camera.hpp"
#include "input.hpp"

template<typename... T>
struct overload : T... {
	using T::operator()...;
};

MTP_ClassicMode::MTP_ClassicMode(Game& game) : GameState(game), connectionState(ConnectionState::Connecting)
{ }

void MTP_ClassicMode::Init() {
	ui.background = std::make_unique<UI>(game.textures.backgroundTexture);

	ui.modeText = std::make_unique<UI>(0, "Multiplayer", 75);
	ui.modeText->textColor = { 1, 1, 0, 1 };

	ui.readyPrompt = std::make_unique<UI>(0, "Press Space to Ready/Cancel");
	ui.readyPrompt->textColor = { 0, 1, 0, 1 };
	
	ui.readyText = std::make_unique<UI>(0, "Ready");
	ui.readyText->textColor = { 0, 1, 0, 1 };
	
	ui.onholdText = std::make_unique<UI>(0, "On Hold");
	ui.onholdText->textColor = { 1, 0.6, 0, 1 };

	ui.connectingText = std::make_unique<UI>(0, "Connecting");
	ui.connectingText->textColor = { 0, 1, 1, 1 };

	ui.disconnectedText = std::make_unique<UI>(0, "Disconnected");
	ui.disconnectedText->textColor = { 1, 0, 0, 1 };
	
	ui.exit = game.createUIObject(game.models.bombModel, { 1, 0, 0, 1 });
	{
		auto slicable = ui.exit->AddComponent<Fruit>(
			MultiplayerSetting.sizeBomb,
			0,
			MultiplayerSetting.fruitSliceForce,
			game.uiConfig.control,
			FruitAsset{}
		);
	}
	ui.exitText = std::make_unique<UI>(0, "Exit");
	ui.exitText->textColor = { 1, 0, 0, 1 };

	ui.reconnect = game.createUIObject(game.models.pineappleModel, { 1, 1, 0, 1 });
	FruitAsset pineappleAsset = {
		game.models.pineappleTopModel,
		game.models.pineappleBottomModel,
		game.audios.fruitSliceAudio1,
		game.audios.fruitMissAudio
	};
	{
		auto slicable = ui.reconnect->AddComponent<Fruit>(
			MultiplayerSetting.sizePineapple,
			0,
			MultiplayerSetting.fruitSliceForce,
			game.uiConfig.control, 
			pineappleAsset
		);
	}
	ui.reconnectText = std::make_unique<UI>(0, "Reconnect");
	ui.reconnectText->textColor = { 1, 1, 0, 1 };

	ui.score1 = std::make_unique<UI>(0, "0");
	ui.score1->textColor = { 0.271f, 0.482f, 0.616f, 1 };
	ui.score2 = std::make_unique<UI>(0, "0");
	ui.score2->textColor = { 0.902f, 0.224f, 0.275f, 1 };
	for (auto i = 0; i < MultiplayerSetting.missTolerence; i++) {
		ui.missFiller1[i] = std::make_unique<UI>(game.textures.redCross);
		ui.missBase1[i] = std::make_unique<UI>(game.textures.emptyCross);
		ui.missBase1[i]->textColor = { 0.271f, 0.482f, 0.616f, 1 };
		ui.missFiller2[i] = std::make_unique<UI>(game.textures.redCross);
		ui.missBase2[i] = std::make_unique<UI>(game.textures.emptyCross);
		ui.missBase2[i]->textColor = { 0.902f, 0.224f, 0.275f, 1 };

		glm::mat4 rotation = glm::rotate(glm::mat4(1), (float)glm::radians(10 + 10.0f * (2 - i)), glm::vec3(0, 0, -1));
		ui.missFiller1[i]->transform.SetRotation(rotation);
		ui.missFiller1[i]->transform.SetScale(glm::vec3(0.1, 0.1, 0.1));
		ui.missBase1[i]->transform.SetRotation(rotation);
		ui.missBase1[i]->transform.SetScale(glm::vec3(0.05, 0.05, 0.05));

		rotation = glm::rotate(glm::mat4(1), -(float)glm::radians(10 + 10.0f * (2 - i)), glm::vec3(0, 0, -1));
		ui.missFiller2[i]->transform.SetRotation(rotation);
		ui.missFiller2[i]->transform.SetScale(glm::vec3(0.1, 0.1, 0.1));
		ui.missBase2[i]->transform.SetRotation(rotation);
		ui.missBase2[i]->transform.SetScale(glm::vec3(0.05, 0.05, 0.05));
	}
}

void MTP_ClassicMode::PositionUI() {
	glm::mat4 inverse = glm::inverse(Camera::main->Perspective() * Camera::main->View());
	float z = Camera::main->ComputerNormalizedZ(30);
	{
		auto pos = inverse * glm::vec4(0.5, -0.5, z, 1);
		pos /= pos.w;
		ui.exit->transform.SetPosition(pos);
	}

	{
		auto pos = inverse * glm::vec4(0, -0.5, z, 1);
		pos /= pos.w;
		ui.reconnect->transform.SetPosition(pos);
	}
}

Coroutine MTP_ClassicMode::FadeInUI(float duration) {
	float time = 0;

	{
		auto rb = ui.exit->GetComponent<Rigidbody>();
		rb->useGravity = false;
		rb->velocity = {};
	}
	{
		auto rb = ui.reconnect->GetComponent<Rigidbody>();
		rb->useGravity = false;
		rb->velocity = {};
	}

	Renderer* exitRenderer = ui.exit->GetComponent<Renderer>();
	Renderer* reconnectRenderer = ui.reconnect->GetComponent<Renderer>();

	while (time < duration) {
		time += game.gameClock.DeltaTime();
		float opacity = time / duration;
		exitRenderer->color.a = opacity;
		exitRenderer->outlineColor.a = opacity;
		reconnectRenderer->color.a = opacity;
		reconnectRenderer->outlineColor.a = opacity;

		ui.modeText->textColor.a = opacity;
		ui.onholdText->textColor.a = opacity;
		ui.readyPrompt->textColor.a = opacity;
		ui.readyText->textColor.a = opacity;
		ui.disconnectedText->textColor.a = opacity;
		ui.connectingText->textColor.a = opacity;
		ui.exitText->textColor.a = opacity;
		ui.reconnectText->textColor.a = opacity;
		co_yield{};
	}
	exitRenderer->color.a = 1;
	exitRenderer->outlineColor.a = 1;
	reconnectRenderer->color.a = 1;
	reconnectRenderer->outlineColor.a = 1;

	ui.modeText->textColor.a = 1;
	ui.onholdText->textColor.a = 1;
	ui.readyPrompt->textColor.a = 1;
	ui.readyText->textColor.a = 1;
	ui.disconnectedText->textColor.a = 1;
	ui.connectingText->textColor.a = 1;
	ui.exitText->textColor.a = 1;
	ui.reconnectText->textColor.a = 1;

	game.uiConfig.control.enableSlicing = true;
}

Coroutine MTP_ClassicMode::FadeOutUI(float duration) {
	float time = 0;

	{
		auto rb = ui.reconnect->GetComponent<Rigidbody>();
		rb->useGravity = true;
	}

	while (time < duration) {
		time += game.gameClock.DeltaTime();
		float opacity = 1 - time / duration;

		ui.modeText->textColor.a = opacity;
		ui.onholdText->textColor.a = opacity;
		ui.readyPrompt->textColor.a = opacity;
		ui.readyText->textColor.a = opacity;
		ui.disconnectedText->textColor.a = opacity;
		ui.connectingText->textColor.a = opacity;
		ui.exitText->textColor.a = opacity;
		ui.reconnectText->textColor.a = opacity;
		co_yield{};
	}

	ui.modeText->textColor.a = 0;
	ui.onholdText->textColor.a = 0;
	ui.readyPrompt->textColor.a = 0;
	ui.readyText->textColor.a = 0;
	ui.disconnectedText->textColor.a = 0;
	ui.connectingText->textColor.a = 0;
	ui.exitText->textColor.a = 0;
	ui.reconnectText->textColor.a = 0;
	game.uiConfig.control.enableSlicing = false;
}

void MTP_ClassicMode::OnEnter() {
	Input::keyCallbacks.emplace(typeid(MTP_ClassicMode), std::bind(&MTP_ClassicMode::RecordKeyboardInput, this, std::placeholders::_1, std::placeholders::_2));
	server = game.connectionManager.ConnectPeer(game.serverAddr, ConnectionTimeOut);
	connectionState = ConnectionState::Connecting;
	currIndex = 0;
	game.manager.Register(ui.exit);
	PositionUI();
	StartCoroutine(FadeInUI(1));
}

void MTP_ClassicMode::OnExit() {
	Input::keyCallbacks.erase(typeid(MTP_ClassicMode));
	ui.exit->Detach();
	if (server) {
		server->Disconnect();
	}
	StartCoroutine(FadeOutUI(1));
}

void MTP_ClassicMode::EnterConnected() {
	inputState = { .index = 0, .keys = 0, .mouseX = 0, .mouseY = 0 };
}

void MTP_ClassicMode::EnterDisconnected() {
	game.manager.Register(ui.reconnect);
}

void MTP_ClassicMode::EnterConnecting() {
	server = game.connectionManager.ConnectPeer(game.serverAddr, ConnectionTimeOut);
}

void MTP_ClassicMode::ProcessServerData() {
	if (server && server->IsConnected()) {
		for (auto pkt = server->Receive(); pkt.has_value(); pkt = server->Receive()) {
			auto serverData = ServerPacket::Deserialize(pkt->data);
			std::visit(
				overload{
					[](std::monostate) { Debug::LogError("Server data failed to deserialize!"); },
					[this](std::tuple<uint64_t, PlayerContext, PlayerContext> contexts){ 
						auto index = std::get<0>(contexts);
						if (index > currIndex) {
							currIndex = index;
							context1 = std::move(std::get<1>(contexts));
							context2 = std::move(std::get<2>(contexts));
						}
					},
					[this](ServerPacket::ServerCommand cmd){ 
						if (cmd == ServerPacket::ServerCommand::StartGame) {
							gameState = InGameState::InGame;
						}
						else if (cmd == ServerPacket::ServerCommand::GameLost) {
							gameState = InGameState::GameLost;
						}
						else if (cmd == ServerPacket::ServerCommand::GameWon) {
							gameState = InGameState::GameWon;
						}
						else if (cmd == ServerPacket::ServerCommand::Disconnect) {
							gameState = InGameState::PlayerDisconnect;
						}
					},
					[](SpawnRequest request) {  }
				}, serverData
			);
		}
	}
}

void MTP_ClassicMode::RecordKeyboardInput(int key, int action) {
	if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
		Debug::Log("Pressed Space");
		inputState.keys = inputState.keys | PlayerKeyPressed::Space;
	}
}

void MTP_ClassicMode::SendInput() {
	if (glfwGetMouseButton(RenderContext::Context->Window(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		inputState.keys = inputState.keys | PlayerKeyPressed::MouseLeft;
	}

	double cursorX, cursorY;
	glfwGetCursorPos(RenderContext::Context->Window(), &cursorX, &cursorY);

	auto dim = RenderContext::Context->Dimension();

	inputState.index++;
	inputState.mouseX = static_cast<float>(cursorX / dim.x);
	inputState.mouseY= static_cast<float>(cursorY / dim.y);
	server->SendData(ClientPacket::Serialize(inputState));
	inputState.keys = PlayerKeyPressed::None;
}

std::optional<std::type_index> MTP_ClassicMode::Step() {
	PositionUI();
	if (!(gameState == InGameState::InGame) && !ui.exit->IsActive()) {
		return {};
	}
	
	switch (connectionState) {
	case ConnectionState::Connecting:
		if (!server || server->IsDisconnected()) {
			connectionState = ConnectionState::Disconnected;
			EnterDisconnected();
		}
		else if (server->IsConnected()) {
			connectionState = ConnectionState::Connected;
			EnterConnected();
		}
		break;

	case ConnectionState::Disconnected:
		if (!ui.reconnect->IsActive()) {
			connectionState = ConnectionState::Connecting;
			EnterConnecting();
		}
		break;

	case ConnectionState::Connected:
		if (!server || server->IsDisconnected()) {
			connectionState = ConnectionState::Disconnected;
			EnterDisconnected();
		}
		else {
			ProcessServerData();
			SendInput();
		}
		break;
	}

	return Self();
}

void MTP_ClassicMode::OnDrawFrontUI(Shader& uiShader) {
	if (!(gameState == InGameState::InGame)) {
		ui.exitText->DrawInNDC({ 0.5, -0.5 }, uiShader);
	}

	if (connectionState == ConnectionState::Connecting) {
		ui.connectingText->DrawInNDC({0, 0}, uiShader);
		return;
	}

	if (connectionState == ConnectionState::Disconnected) {
		ui.disconnectedText->DrawInNDC({ 0, 0 }, uiShader);
		ui.reconnectText->DrawInNDC({0, -0.5}, uiShader);
		return;
	}

	if (gameState != InGameState::InGame) {
		if (context1.isReady) {
			ui.readyText->DrawInNDC({ -0.5, -0.5 }, uiShader);
		}
		else {
			ui.onholdText->DrawInNDC({ -0.5, -0.5 }, uiShader);
		}
		ui.readyPrompt->DrawInNDC({ 0, 0 }, uiShader);
		return;
	}

	ui.score1->DrawInNDC({-0.5f, 0.9f}, uiShader);
	ui.score2->DrawInNDC({0.5f, 0.9f}, uiShader);

	for (int i = 0; i < MultiplayerSetting.missTolerence; i++) {
		if (i < context1.numMisses) {
			ui.missFiller1[i]->DrawInNDC({-0.95f + i * 0.06, 0.8f + i * 0.04}, uiShader);
			ui.missFiller2[i]->DrawInNDC({0.95f - i * 0.06, 0.8f + i * 0.04 }, uiShader);
		}
		else {
			ui.missBase1[i]->DrawInNDC({ -0.95f + i * 0.06, 0.8f + i * 0.04 }, uiShader);
			ui.missBase2[i]->DrawInNDC({ 0.95f - i * 0.06, 0.8f + i * 0.04 }, uiShader);
		}
	}
}

void MTP_ClassicMode::OnDrawBackUI(Shader& uiShader) {
	ui.modeText->DrawInNDC({ 0, 0.75 }, uiShader);
}

bool MTP_ClassicMode::AllowParentFrontUI() {
	return false;
}
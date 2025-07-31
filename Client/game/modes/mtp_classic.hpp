#ifndef MTP_CLASSIC_H
#define MTP_CLASSIC_H
#include <infrastructure/ui.hpp>
#include "multiplayer/game_packet.hpp"
#include "game/game.hpp"

struct MTP_ClassicUI {
	// Background
	std::unique_ptr<UI> background;
	std::unique_ptr<UI> modeText;

	// Buttons
	std::shared_ptr<Object> exit;
	std::unique_ptr<UI> exitText;

	std::shared_ptr<Object> reconnect;
	std::unique_ptr<UI> reconnectText;

	// Front
	std::unique_ptr<UI> readyText;
	std::unique_ptr<UI> onholdText;
	std::unique_ptr<UI> readyPrompt;
	std::unique_ptr<UI> disconnectedText;
	std::unique_ptr<UI> connectingText;
};

class MTP_ClassicMode : public GameState {
	std::shared_ptr<LiteConnConnection> server;
	MTP_ClassicUI ui;
	enum {
		Connecting,
		Disconnected,
		Connected
	} state;
	PlayerInputState inputState = {.index = 0, .keys = 0, .mouseX = 0, .mouseY = 0};

	void EnterConnecting();
	void EnterDisconnected();
	void EnterConnected();
	void PositionUI();
	void RecordKeyboardInput(int key, int action);
	void SendInput();
	Coroutine FadeInUI(float duration);
	Coroutine FadeOutUI(float duration);
public:
	MTP_ClassicMode(Game& game);
	void Init() override;
	void OnEnter() override;
	void OnExit() override;
	std::optional<std::type_index> Step() override;
	
	void OnDrawBackUI(Shader& uiShader) override;
	void OnDrawFrontUI(Shader& uiShader) override;
};
#endif
#ifndef STATE_SELECTION_H
#define STATE_SELECTION_H
#include "infrastructure/ui.hpp"
#include "audio/audiosource.hpp"
#include "game/game.hpp"

class SelectionState : public GameState {
	struct {
		std::unique_ptr<UI> classicModeText;
		std::unique_ptr<UI> multiplayerText;
		std::unique_ptr<UI> exitText;
		std::shared_ptr<Object> classicModeSelection;
		std::shared_ptr<Object> multiplayerSelection;
		std::shared_ptr<Object> exitSelection;

		std::unique_ptr<UI> backgroundImage;
		std::unique_ptr<UI> titleText;
	} ui;
	AudioSource* music = {};

	Coroutine FadeInUI(float duration);
	Coroutine FadeOutUI(float duration);
	void PositionUI();
public:
	SelectionState(Game* game);
	void Init() override;
	void OnEnter() override;
	void OnExit() override;
	void OnEnterSubState() override;
	void OnExitSubState() override;
	void OnDrawFrontUI(Shader& uiShader) override;
	void OnDrawBackUI(Shader& uiShader) override;
	std::optional<std::type_index> Step() override;
};

#endif
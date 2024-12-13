#ifndef LIVE_INPUT_STATE
#define LIVE_INPUT_STATE

struct LiveInputState {
	bool left_pressed = false;
	bool right_pressed = false;
	bool forward_pressed = false;
	bool backward_pressed = false;
	bool jump_pressed = false;
	bool fire_pressed = false;
	double mouse_position_x = 0.0, mouse_position_y = 0.0;
};

#endif // LIVE_INPUT_STATE
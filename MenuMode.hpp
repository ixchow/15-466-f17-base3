#pragma once

#include "Mode.hpp"

struct MenuMode : public Mode {
	virtual ~MenuMode() { }

	virtual bool handle_event(SDL_Event const &);
	virtual void update(float elapsed);
	virtual void draw();

	float spin = 0.0f;
};

#pragma once

#include <SDL.h>
#include <glm/glm.hpp>

#include <memory>

class Mode : public std::enable_shared_from_this< Mode > {
public:
	virtual ~Mode() { }

	//NOTE: return value of handle_event indicates if the event was handled
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) { return false; }
	virtual void update(float elapsed) { }
	virtual void draw(glm::uvec2 const &size) = 0;

	static std::shared_ptr< Mode > current;
	static void set_current(std::shared_ptr< Mode > const &);
};


#pragma once

#include "Mode.hpp"

#include "Scene.hpp"

#include <functional>

struct GameMode : public Mode {
	GameMode();
	virtual ~GameMode() { }

	virtual bool handle_event(SDL_Event const &event, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	void reset(); //reset the game state

	//scene + references into scene for objects:
	Scene scene;

	struct Goal {
		Goal(Scene::Transform *transform_) : transform(transform_) { }
		Scene::Transform *transform = nullptr;
		float radius = 0.4f;
	};
	std::vector< Goal > goals;

	struct Ball {
		Ball(std::string const &name, Scene::Transform *transform_);
		Scene::Transform *transform = nullptr;
		float radius = 0.15f;
		enum Type {
			Diamond,
			Solid,
			Eight
		} type;
		glm::vec2 velocity = glm::vec2(0.0f);
		bool scored = false;

		glm::vec3 init_position;
		glm::quat init_rotation;
	};
	std::vector< Ball > balls;

	struct Dozer {
		Scene::Transform *transform = nullptr;
		float radius = 0.15f;

		float heading = 0.0f; //direction the dozer is facing, in radians

		float left_tread = 0.0f; //tread speeds in range [-1.0,1.0]
		float right_tread = 0.0f;

		glm::vec2 velocity = glm::vec2(0.0f); //used when resolving collisions

		glm::vec3 init_position;
		glm::quat init_rotation;
	};
	Dozer diamond_dozer, solid_dozer;


	//controls:
	struct Controls {
		SDL_Scancode left_forward;
		SDL_Scancode left_backward;
		SDL_Scancode right_forward;
		SDL_Scancode right_backward;
	};
	Controls diamond_controls = {SDL_SCANCODE_A, SDL_SCANCODE_Z, SDL_SCANCODE_S, SDL_SCANCODE_X};
	Controls solid_controls = {SDL_SCANCODE_SEMICOLON, SDL_SCANCODE_PERIOD, SDL_SCANCODE_APOSTROPHE, SDL_SCANCODE_SLASH};

	//function to call when 'ESC' is pressed:
	std::function< void() > show_menu;

	bool did_end = false;

	//functions to call when game ends:
	std::function< void() > diamonds_wins;
	std::function< void() > solids_wins;
	std::function< void() > everyone_loses;

};

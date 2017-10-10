#include "GameMode.hpp"

#include "Load.hpp"
#include "read_chunk.hpp"
#include "MeshBuffer.hpp"
#include "GLProgram.hpp"
#include "GLVertexArray.hpp"

#include <fstream>

Load< MeshBuffer > pool_meshes(LoadTagInit, [](){
	return new MeshBuffer("pool.pnc");
});

/*
NOTE: something like this eventually:

void main() {
	highp float shininess = pow(1024.0, 1.0 - uRoughness);

	highp vec3 n = normalize(vNormal);
	highp vec3 v = normalize(uCamera - vPosition);

	//--------------
	//Distant directional light:

	highp vec3 l = normalize(uLightPosition - vec3(0.0));
	highp vec3 lightEnergy = uLightEnergy;

	//--------------

	highp vec3 h = normalize(l + v);

	//--------------

	//Basic Physically-inspired BRDF:
	highp vec3 reflectance =
		vColor.rgb / 3.1415926 //Lambertain Diffuse
		+ pow(max(0.0, dot(n, h)), shininess) //Blinn-Phong Specular
		  * (shininess + 2.0) / (8.0) //normalization factor
		  * (0.04 + (1.0 - 0.04) * pow(1.0 - dot(l,h), 5.0)) //Schlick's approximation for Fresnel reflectance
	;

	highp vec3 lightFlux = lightEnergy * max(0.0, dot(n, l));
	highp vec3 color = reflectance * lightFlux;

	//--------------

	gl_FragColor = vec4(color, 1.0);
}
*/

//Attrib locations in game_program:
GLint game_program_Position = -1;
GLint game_program_Normal = -1;
GLint game_program_Color = -1;
//Uniform locations in game_program:
GLint game_program_mvp = -1;
GLint game_program_mv = -1;
GLint game_program_itmv = -1;

//Menu program itself:
Load< GLProgram > game_program(LoadTagInit, [](){
	GLProgram *ret = new GLProgram(
		"#version 330\n"
		"uniform mat4 mvp;\n" //model positions to clip space
		"uniform mat4x3 mv;\n" //model positions to lighting space
		"uniform mat3 itmv;\n" //model normals to lighting space
		"in vec4 Position;\n"
		"in vec3 Normal;\n"
		"in vec3 Color;\n"
		"out vec3 color;\n"
		"out vec3 normal;\n"
		"out vec3 position;\n"
		"void main() {\n"
		"	gl_Position = mvp * Position;\n"
		"	position = mv * Position;\n"
		"	normal = itmv * Normal;\n"
		"	color = Color;\n"
		"}\n"
	,
		"#version 330\n"
		"in vec3 color;\n"
		"in vec3 normal;\n"
		"in vec3 position;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"	float roughness = 0.5;\n"
		"	float shininess = pow(1024.0, 1.0 - roughness);\n"
		"	vec3 n = normalize(normal);\n"
		"	vec3 l = vec3(0.0, 0.0, 1.0);\n"
		"	l = normalize(mix(l, n, 0.4));\n" //hemisphere light
		"	vec3 h = normalize(n+l);\n"
		"	vec3 reflectance = \n"
		"		color.rgb / 3.1415926 //Lambertain Diffuse \n"
		"		+ pow(max(0.0, dot(n, h)), shininess) //Blinn-Phong Specular \n"
		"		* (shininess + 2.0) / (8.0) //normalization factor \n"
		"		* (0.04 + (1.0 - 0.04) * pow(1.0 - dot(l,h), 5.0)) //Schlick's approximation for Fresnel reflectance \n"
		"	;\n"
		"	fragColor = vec4(max(dot(n,l),0.0) * 1.5 * reflectance, 1.0);\n"
		"}\n"
	);

	//TODO: change these to be errors once full shader is in place
	game_program_Position = (*ret)("Position");
	game_program_Normal = ret->getAttribLocation("Normal", GLProgram::MissingIsWarning);
	game_program_Color = ret->getAttribLocation("Color", GLProgram::MissingIsWarning);
	game_program_mvp = (*ret)["mvp"];
	game_program_mv = ret->getUniformLocation("mv", GLProgram::MissingIsWarning);
	game_program_itmv = ret->getUniformLocation("itmv", GLProgram::MissingIsWarning);

	return ret;
});

//Binding for using game_program on pool_meshes:
Load< GLVertexArray > pool_binding(LoadTagDefault, [](){
	GLVertexArray *ret = new GLVertexArray(GLVertexArray::make_binding(game_program->program, {
		{game_program_Position, pool_meshes->Position},
		{game_program_Normal, pool_meshes->Normal},
		{game_program_Color, pool_meshes->Color}
	}));
	return ret;
});

//------------------------------

GameMode::Ball::Ball(std::string const &name, Scene::Transform *transform_) : transform(transform_) {
	assert(transform);

	int32_t number = std::stoi(name.substr(5));
	if (number < 8) type = Solid;
	else if (number == 8) type = Eight;
	else type = Diamond;

	init_position = transform->position;
	init_rotation = transform->rotation;
}

GameMode::GameMode() {
	auto add_object = [&](std::string const &name, glm::vec3 const &position, glm::quat const &rotation, glm::vec3 const &scale) {
		scene.objects.emplace_back();
		Scene::Object &object = scene.objects.back();
		object.transform.position = position;
		object.transform.rotation = rotation;
		object.transform.scale = scale;

		MeshBuffer::Mesh const &mesh = pool_meshes->lookup(name);
		object.program = game_program->program;
		object.program_mvp = game_program_mvp;
		object.program_mv = game_program_mv;
		object.program_itmv = game_program_itmv;
		object.vao = pool_binding->array;
		object.start = mesh.start;
		object.count = mesh.count;

		if (name == "Dozer.Diamond") {
			assert(!diamond_dozer.transform);
			diamond_dozer.transform = &object.transform;
			diamond_dozer.init_position = object.transform.position;
			diamond_dozer.init_rotation = object.transform.rotation;
		} else if (name == "Dozer.Solid") {
			assert(!solid_dozer.transform);
			solid_dozer.transform = &object.transform;
			solid_dozer.heading = glm::roll(object.transform.rotation);
			solid_dozer.init_position = object.transform.position;
			solid_dozer.init_rotation = object.transform.rotation;
		} else if (name.substr(0, 5) == "Ball-") {
			balls.emplace_back(name, &object.transform);
		} else if (name.substr(0, 5) == "Goal.") {
			goals.emplace_back(&object.transform);
		} else {
			//this object must be scenery
		}

	};

	{ //read scene "pool.scene":
		std::ifstream file("pool.scene", std::ios::binary);

		std::vector< char > strings;
		//read strings chunk:
		read_chunk(file, "str0", &strings);

		{ //read scene chunk, add meshes to scene:
			struct SceneEntry {
				uint32_t name_begin, name_end;
				glm::vec3 position;
				glm::quat rotation;
				glm::vec3 scale;
			};
			static_assert(sizeof(SceneEntry) == 48, "Scene entry should be packed");

			std::vector< SceneEntry > data;
			read_chunk(file, "scn0", &data);

			for (auto const &entry : data) {
				if (!(entry.name_begin <= entry.name_end && entry.name_end <= strings.size())) {
					throw std::runtime_error("index entry has out-of-range name begin/end");
				}
				std::string name(&strings[0] + entry.name_begin, &strings[0] + entry.name_end);
				add_object(name, entry.position, entry.rotation, entry.scale);
			}
		}
	}

	//If the "scene" file is proper, these transforms would be hooked up:
	assert(diamond_dozer.transform);
	assert(solid_dozer.transform);
	assert(balls.size() == 15);
	assert(goals.size() == 6);

	//set up for first round:
	reset();
}

void GameMode::reset() {
	did_end = false;

	for (auto &ball : balls) {
		ball.transform->position = ball.init_position;
		ball.transform->rotation = ball.init_rotation;
		ball.transform->scale = glm::vec3(1.0f);
		ball.velocity = glm::vec2(0.0f);
		ball.scored = false;
	}
	for (auto &dozer : {&diamond_dozer, &solid_dozer}) {
		dozer->transform->position = dozer->init_position;
		dozer->transform->rotation = dozer->init_rotation;
		dozer->velocity = glm::vec2(0.0f);
		dozer->heading = glm::roll(dozer->init_rotation);
		dozer->left_tread = 0.0f;
		dozer->right_tread = 0.0f;
	}
}


bool GameMode::handle_event(SDL_Event const &e, glm::uvec2 const &window_size) {
	if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
		if (show_menu) show_menu();
		return true;
	}
	auto matches = [&e](Controls const &c) {
		return ((e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) && (
			e.key.keysym.scancode == c.left_forward
			|| e.key.keysym.scancode == c.left_backward
			|| e.key.keysym.scancode == c.right_forward
			|| e.key.keysym.scancode == c.right_backward
		));
	};
	if (matches(diamond_controls) || matches(solid_controls)) {
		return true;
	}
	return false;
}

void GameMode::update(float elapsed) {
	static Uint8 const *keys = SDL_GetKeyboardState(NULL);

	//move dozers:
	auto do_dozer = [&](Dozer &dozer, Controls const &controls) {
		
		{ //tread acceleration:
			float left = 0.0f;
			float right = 0.0f;
			if (!did_end) {
				if (keys[controls.left_forward]) left += 1.0f;
				if (keys[controls.left_backward]) left -= 1.0f;
				if (keys[controls.right_forward]) right += 1.0f;
				if (keys[controls.right_backward]) right -= 1.0f;
			}

			const float TreadAcc = 5.0f;
			if (dozer.left_tread < left) dozer.left_tread = std::min(dozer.left_tread + elapsed * TreadAcc, left);
			if (dozer.left_tread > left) dozer.left_tread = std::max(dozer.left_tread - elapsed * TreadAcc, left);
			if (dozer.right_tread < right) dozer.right_tread = std::min(dozer.right_tread + elapsed * TreadAcc, right);
			if (dozer.right_tread > right) dozer.right_tread = std::max(dozer.right_tread - elapsed * TreadAcc, right);
		}

		//want velocity 'left' at -tread_offset and 'right' at +tread_offset:
		constexpr float TreadSpeed = 0.4f;
		constexpr float Offset = 0.067f;
		float forward_vel = 0.5f * (TreadSpeed * dozer.left_tread + TreadSpeed * dozer.right_tread);
		float rot_vel = (TreadSpeed * dozer.right_tread - forward_vel) / Offset;

		glm::vec2 forward = glm::vec2(std::cos(dozer.heading), std::sin(dozer.heading));
		dozer.velocity = forward * forward_vel;

		dozer.transform->position.x += dozer.velocity.x * elapsed;
		dozer.transform->position.y += dozer.velocity.y * elapsed;

		dozer.heading += rot_vel * elapsed;

		dozer.transform->rotation = glm::angleAxis(dozer.heading, glm::vec3(0.0f, 0.0f, 1.0f));
	};
	do_dozer(diamond_dozer, diamond_controls);
	do_dozer(solid_dozer, solid_controls);

	//move balls:
	for (auto &ball : balls) {
		if (ball.scored) {
			float s = glm::max(0.0f, ball.transform->scale.x - elapsed);
			ball.transform->scale = glm::vec3(s);
			ball.transform->position.z = ball.radius * s;
		}
		ball.velocity *= std::pow(0.5f, elapsed / 5.0f);
		constexpr float friction = 0.1f;
		if (ball.velocity.x > 0.0f) ball.velocity.x = std::max(0.0f, ball.velocity.x - friction * elapsed);
		if (ball.velocity.x < 0.0f) ball.velocity.x = std::min(0.0f, ball.velocity.x + friction * elapsed);
		if (ball.velocity.y > 0.0f) ball.velocity.y = std::max(0.0f, ball.velocity.y - friction * elapsed);
		if (ball.velocity.y < 0.0f) ball.velocity.y = std::min(0.0f, ball.velocity.y + friction * elapsed);
		glm::vec2 delta = elapsed * ball.velocity * ball.transform->scale.x;
		ball.transform->position.x += delta.x;
		ball.transform->position.y += delta.y;
		//estimate rotation:
		if (delta != glm::vec2(0.0f)) {
			glm::vec3 axis = glm::normalize(glm::vec3(-delta.y, delta.x, 0.0f));
			float angle = glm::length(delta) / ball.radius;
			ball.transform->rotation = glm::normalize(glm::angleAxis(angle, axis) * ball.transform->rotation);
		}
	}


	{ //Resolve collisions:
		auto resolve = [](glm::vec3 &a, glm::vec2 &va, glm::vec3 &b, glm::vec2 &vb, float distance, float ratio, float restitution) {
			glm::vec2 to = glm::vec2(b) - glm::vec2(a);
			float len = glm::dot(to, to);
			if (len < distance*distance) {
				len = std::sqrt(len);
				float amt = distance - len;
				if (len < 0.001f) {
					amt = 0.1f * distance;
					to = glm::vec2(1.0f, 0.0f);
				} else {
					to /= len;
				}
				a -= glm::vec3( (ratio * amt) * to, 0.0f );
				b += glm::vec3( ((1.0f - ratio) * amt) * to, 0.0f );

				//relative velocity:
				float rv = glm::dot(to, vb - va);
				if (rv < 0.0f) {
					float want = std::abs(rv) * restitution;
					va -= (ratio * (want - rv)) * to;
					vb += ((1.0f - ratio) * (want - rv)) * to;
				}
				return true;
			}
			return false;
		};

		//dozer vs dozer:
		resolve(diamond_dozer.transform->position, diamond_dozer.velocity, solid_dozer.transform->position, solid_dozer.velocity, diamond_dozer.radius + solid_dozer.radius, 0.5f, 0.0f);

		//dozer vs balls:
		for (auto dozer : {&diamond_dozer, &solid_dozer}) {
			for (auto &ball : balls) {
				if (ball.scored) continue;
				resolve(
					dozer->transform->position, dozer->velocity,
					ball.transform->position, ball.velocity,
					dozer->radius + ball.radius, 0.0f, 0.7f);
			}
		}

		//balls vs balls:
		for (auto &a : balls) {
			if (a.scored) continue;
			for (auto &b : balls) {
				if (&a == &b) break;
				if (b.scored) continue;
				resolve(
					a.transform->position, a.velocity,
					b.transform->position, b.velocity,
					a.radius + b.radius, 0.5f, 0.7f);
			}
		}

		//dozer vs walls:
		for (auto dozer : {&diamond_dozer, &solid_dozer}) {
			if (dozer->transform->position.x < -3.0f + dozer->radius) {
				dozer->transform->position.x = -3.0f + dozer->radius;
				dozer->velocity.x = std::max(0.0f,dozer->velocity.x);
			}
			if (dozer->transform->position.x > 3.0f - dozer->radius) {
				dozer->transform->position.x = 3.0f - dozer->radius;
				dozer->velocity.x = std::min(0.0f, dozer->velocity.x);
			}
			if (dozer->transform->position.y < -2.0f + dozer->radius) {
				dozer->transform->position.y = -2.0f + dozer->radius;
				dozer->velocity.y = std::max(0.0f,dozer->velocity.y);
			}
			if (dozer->transform->position.y > 2.0f - dozer->radius) {
				dozer->transform->position.y = 2.0f - dozer->radius;
				dozer->velocity.y = std::min(0.0f, dozer->velocity.y);
			}
		}

		//balls vs walls:
		for (auto &ball : balls) {
			if (ball.scored) continue;
			constexpr float restitution = 0.7f;
			if (ball.transform->position.x < -3.0f + ball.radius) {
				ball.transform->position.x = -3.0f + ball.radius;
				if (ball.velocity.x < 0.0f) ball.velocity.x = restitution * std::abs(ball.velocity.x);
			}
			if (ball.transform->position.x > 3.0f - ball.radius) {
				ball.transform->position.x = 3.0f - ball.radius;
				if (ball.velocity.x > 0.0f) ball.velocity.x =-restitution * std::abs(ball.velocity.x);
			}
			if (ball.transform->position.y < -2.0f + ball.radius) {
				ball.transform->position.y = -2.0f + ball.radius;
				if (ball.velocity.y < 0.0f) ball.velocity.y = restitution * std::abs(ball.velocity.y);
			}
			if (ball.transform->position.y > 2.0f - ball.radius) {
				ball.transform->position.y = 2.0f - ball.radius;
				if (ball.velocity.y > 0.0f) ball.velocity.y =-restitution * std::abs(ball.velocity.y);
			}
		}
	}

	//Check scoring:
	for (auto &goal : goals) {
		glm::vec2 g = glm::vec2(goal.transform->position);
		for (auto &ball : balls) {
			if (ball.scored) continue;
			glm::vec2 b = glm::vec2(ball.transform->position);
			if (glm::dot(g-b, g-b) < goal.radius * goal.radius) {
				//score!
				ball.scored = true;
			}
		}
	}

	{ //Check end conditions:
		uint32_t diamonds_remain = 0;
		uint32_t solids_remain = 0;
		uint32_t eights_sunk = 0;
		for (auto &ball : balls) {
			if (ball.type == Ball::Diamond) {
				diamonds_remain += (ball.scored ? 0 : 1);
			} else if (ball.type == Ball::Solid) {
				solids_remain += (ball.scored ? 0 : 1);
			} else { assert(ball.type == Ball::Eight);
				eights_sunk += (ball.scored ? 1 : 0);
			}
		}
		//DEBUG: std::cout << diamonds_remain << "d + " << solids_remain << "s" << std::endl;
		if (eights_sunk != 0 && !did_end) {
			did_end = true;
			if (diamonds_remain == 0) {
				std::cout << "Diamonds wins." << std::endl;
				if (diamonds_wins) diamonds_wins();
			} else if (solids_remain == 0) {
				std::cout << "Solids wins." << std::endl;
				if (solids_wins) solids_wins();
			} else {
				std::cout << "Everyone loses." << std::endl;
				if (everyone_loses) everyone_loses();
			}
		}
	}


}

void GameMode::draw(glm::uvec2 const &drawable_size) {
	scene.camera.aspect = drawable_size.x / float(drawable_size.y);
	scene.camera.fovy = 25.0f / 180.0f * 3.1415926f;
	scene.camera.transform.rotation = glm::angleAxis(17.0f / 180.0f * 3.1415926f, glm::vec3(1.0f, 0.0f, 0.0f));
	scene.camera.transform.position = glm::vec3(0.0f, -3.3f, 10.49f);
	scene.camera.near = 4.0f;

	scene.render();
}



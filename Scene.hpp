#pragma once

#include "GL.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <list>
#include <functional>

//Describes a 3D scene for rendering:
struct Scene {
	struct Transform {
		Transform() = default;
		Transform(Transform &) = delete;
		~Transform() {
			while (last_child) {
				last_child->set_parent(nullptr);
			}
			if (parent) {
				set_parent(nullptr);
			}
		}

		//simple specification:
		glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::quat rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);

		//hierarchy information:
		Transform *parent = nullptr;
		Transform *last_child = nullptr;
		Transform *prev_sibling = nullptr;
		Transform *next_sibling = nullptr;
		//Generally, you shouldn't manipulate the above pointers directly.

		//Add transform to the child list of 'parent', before child 'before':
		void set_parent(Transform *parent, Transform *before = nullptr);

		//helper that checks local pointer consistency:
		void DEBUG_assert_valid_pointers() const;

		//computed from the above:
		glm::mat4 make_local_to_parent() const;
		glm::mat4 make_parent_to_local() const;
		glm::mat4 make_local_to_world() const;
		glm::mat4 make_world_to_local() const;
	};
	struct Camera {
		Transform transform; //cameras look along their -z axis
		//camera parameters (perspective):
		float fovy = glm::radians(60.0f); //vertical fov (in radians)
		float aspect = 1.0f; //x / y
		float near = 0.01f; //near plane
		//computed from the above:
		glm::mat4 make_projection() const;
	};
	struct Object {
		Transform transform;
		//program info:
		GLuint program = 0;
		GLuint program_mvp = -1U; //uniform index for MVP matrix (mat4)
		GLuint program_mv = -1U; //uniform index for model-to-lighting-space matrix (mat4x3)
		GLuint program_itmv = -1U; //uniform index for normal-to-lighting-space matrix (mat3)

		//material info:
		std::function< void(Object const &) > set_uniforms; //will be called before rendering object

		//attribute info:
		GLuint vao = 0;
		GLuint start = 0;
		GLuint count = 0;
	};
	struct Light {
		Transform transform; //lights (at least, lights that point) point along their -z axis
		//light parameters:
		glm::vec3 intensity = glm::vec3(1.0f, 1.0f, 1.0f); //effectively, color
	};

	Camera camera;
	std::list< Object > objects;
	std::list< Light > lights;

	void render();
};

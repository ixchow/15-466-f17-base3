#include "MenuMode.hpp"

#include "Load.hpp"
#include "GLProgram.hpp"
#include "GLVertexArray.hpp"
#include "Meshes.hpp"

#include <cmath>

//---------- resources ------------
Load< Meshes > menu_meshes(LoadTagInit, [](){
	Meshes *ret = new Meshes();
	ret->load("menu.p");
	return ret;
});

//Attrib locations in menu_program:
GLint menu_program_Position = -1;
GLint menu_program_Normal = -1;
//Uniform locations in menu_program:
GLint menu_program_mvp = -1;
GLint menu_program_itmv = -1;
GLint menu_program_to_light = -1;

//Menu program itself:
Load< GLProgram > menu_program(LoadTagInit, [](){
	GLProgram *ret = new GLProgram(
		"#version 330\n"
		"uniform mat4 mvp;\n"
		"uniform mat3 itmv;\n"
		"in vec4 Position;\n"
		"in vec3 Normal;\n"
		"out vec3 normal;\n"
		"void main() {\n"
		"	gl_Position = mvp * Position;\n"
		"	normal = itmv * Normal;\n"
		"}\n"
	,
		"#version 330\n"
		"uniform vec3 to_light;\n"
		"in vec3 normal;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"	float light = max(0.0, dot(normalize(normal), to_light));\n"
		"	fragColor = vec4(light * vec3(1.0, 1.0, 1.0), 1.0);\n"
		"}\n"
	);

	menu_program_Position = (*ret)("Position");
	menu_program_Normal = (*ret)("Normal");
	menu_program_mvp = (*ret)["mvp"];
	menu_program_itmv = (*ret)["itmv"];
	menu_program_to_light = (*ret)["to_light"];

	return ret;
});

//Binding for using menu_program on menu_meshes:
Load< GLVertexArray > menu_binding(LoadTagDefault, [](){
	menu_program_Position = (*menu_program)("Position"); //DEBUG!
	menu_program_Normal = (*menu_program)("Normal");
	Mesh const &x = menu_meshes->get("X");
	GLVertexArray *ret = new GLVertexArray(GLVertexArray::make_binding(menu_program->program, {
		{menu_program_Position, x.Position},
		{menu_program_Normal, x.Normal},
	}));
	return ret;
});


//----------------------

bool MenuMode::handle_event(SDL_Event const &e) {
	return false;
}

void MenuMode::update(float elapsed) {
	spin += elapsed / 10.0f;
	spin -= std::floor(spin);
}

void MenuMode::draw() {

}

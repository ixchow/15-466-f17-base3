#pragma once

#include "GLBuffer.hpp"

#include "GL.hpp"
#include <map>

//Mesh is a lightweight handle to some OpenGL vertex data:
struct Mesh {
	GLAttribPointer Position;
	GLAttribPointer Normal;
	GLAttribPointer Color;
	GLAttribPointer TexCoord;
	GLuint start = 0;
	GLuint count = 0;
};

//"Meshes" loads a collection of meshes and builds 'Mesh' objects for them:
// (note that meshes in a single collection will share a vao)

struct Meshes {
	//add meshes from a file:
	// note: will throw if file fails to read.
	void load(std::string const &filename);

	//look up a particular mesh in the DB:
	// note: will throw if mesh not found.
	Mesh const &get(std::string const &name) const;

	//internals:
	std::map< std::string, Mesh > meshes;
};

#pragma once

#include "GLBuffer.hpp"

#include "GL.hpp"
#include <map>

//"MeshBuffer" holds a collection of meshes loaded from a file
// (note that meshes in a single collection will share a vao)

struct MeshBuffer {
	GLAttribPointer Position;
	GLAttribPointer Normal;
	GLAttribPointer Color;
	GLAttribPointer TexCoord;

	GLBuffer buffer;

	//construct from a file:
	// note: will throw if file fails to read.
	MeshBuffer(std::string const &filename);

	//look up a particular mesh in the DB:
	// note: will throw if mesh not found.
	struct Mesh {
		GLuint start = 0;
		GLuint count = 0;
	};
	const Mesh &lookup(std::string const &name) const;

	//internals:
	std::map< std::string, Mesh > meshes;
};

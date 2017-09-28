#pragma once

/*
 * GLProgram is a wrapper for an OpenGL Shader Program.
 * It provides some convenience methods to perform *checked*
 *  lookups of attributes and uniforms.
 *
 */

#include "GL.hpp"

#include <string>
#include <stdexcept>

struct GLProgram {
	//Compiles + links program from source; throws on compile error:
	GLProgram(
		std::string const &vertex_source,
		std::string const &fragment_source
	);
	~GLProgram();
	GLuint program = 0;

	//operator() looks up an attribute (and checks that it exists):
	GLint operator()(std::string const &name) const {
		GLint ret = glGetAttribLocation(program, name.c_str());
		if (ret == -1) {
			throw std::runtime_error("Attribute '" + name + "' does not exist in program.");
		}
		return ret;
	}

	//operator[] looks up a uniform address (and checks that it exists):
	GLint operator[](std::string const &name) const {
		GLint ret = glGetUniformLocation(program, name.c_str());
		if (ret == -1) {
			throw std::runtime_error("Uniform '" + name + "' does not exist in program.");
		}
		return ret;
	}
};

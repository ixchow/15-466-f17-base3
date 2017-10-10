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
#include <iostream>

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
		return getAttribLocation(name, MissingIsError);
	}

	enum MissingIs {
		MissingIsWarning = 0,
		MissingIsError = 1,
	};

	//getAttribLocation looks up an attribute:
	GLint getAttribLocation(std::string const &name, MissingIs missing_is = MissingIsError) const {
		GLint ret = glGetAttribLocation(program, name.c_str());
		if (ret == -1) {
			if (missing_is == MissingIsWarning) {
				std::cerr << "WARNING: Attribute '" + name + "' does not exist in program." << std::endl;
			} else {
				throw std::runtime_error("Attribute '" + name + "' does not exist in program.");
			}
		}
		return ret;
	}


	//operator[] looks up a uniform address (and checks that it exists):
	GLint operator[](std::string const &name) const {
		return getUniformLocation(name, MissingIsError);
	}

	//getUniformLocation looks up a uniform address:
	GLint getUniformLocation(std::string const &name, MissingIs missing_is = MissingIsError) const {
		GLint ret = glGetUniformLocation(program, name.c_str());
		if (ret == -1) {
			if (missing_is == MissingIsWarning) {
				std::cerr << "WARNING: Uniform '" + name + "' does not exist in program." << std::endl;
			} else {
				throw std::runtime_error("Uniform '" + name + "' does not exist in program.");
			}
		}
		return ret;
	}

};

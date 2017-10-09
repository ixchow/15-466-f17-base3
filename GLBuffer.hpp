#pragma once

/*
 * This header declares classes that wrap GL Buffer objects.
 * Specifically:
 * A GLBuffer just wraps a buffer object; everything else is up to you.
 * A GLAttribBuffer< > wraps a vertex attribute buffer, and remembers both
 *  the type of the attributes stored in that buffer and how many vertices are stored.
 *
 */

#include "GL.hpp"

#include <glm/glm.hpp>

#include <vector>

//GLBuffer is a thin wrapper around a buffer:
struct GLBuffer {
	GLuint buffer = 0;

	GLBuffer() { glGenBuffers(1, &buffer); }
	~GLBuffer() { if (buffer != 0) glDeleteBuffers(1, &buffer); }
	GLBuffer(GLBuffer const &) = delete;
	GLBuffer(GLBuffer &&from) { std::swap(buffer, from.buffer); }
	GLBuffer &operator=(GLBuffer &&from) { std::swap(buffer, from.buffer); return *this; }

	void set(GLenum target, GLsizeiptr size, GLvoid const *data, GLenum usage) {
		glBindBuffer(target, buffer);
		glBufferData(target, size, data, usage);
	}
};

//AttribPointer identifies a location within a buffer:
struct GLAttribPointer {
	GLuint buffer = 0;

	GLint size = 0;
	GLenum type = 0;
	GLboolean normalized = GL_FALSE;
	GLsizei stride = 0;
	GLsizei offset = 0; //offset into the buffer, in bytes

	GLAttribPointer() = default;
	GLAttribPointer(GLuint _buffer, GLint _size, GLenum _type, GLboolean _normalized, GLsizei _stride, GLsizei _offset)
		: buffer(_buffer), size(_size), type(_type), normalized(_normalized), stride(_stride), offset(_offset) { }
};

//Traits struct that stores GL info about c++/glm types:
template< typename T >
struct GLTypeInfo;

#define SPECIALIZE( GLM_TYPE, GL_SIZE, GL_TYPE, GL_NORMALIZED ) \
	template< > \
	struct GLTypeInfo< GLM_TYPE > { \
		enum : GLint { size = GL_SIZE }; \
		enum : GLenum { type = GL_TYPE }; \
		enum : GLboolean { normalized = GL_NORMALIZED }; \
	};

SPECIALIZE( float, 1, GL_FLOAT, GL_FALSE );
SPECIALIZE( glm::vec2, 2, GL_FLOAT, GL_FALSE );
SPECIALIZE( glm::vec3, 3, GL_FLOAT, GL_FALSE );
SPECIALIZE( glm::vec4, 4, GL_FLOAT, GL_FALSE );
SPECIALIZE( glm::u8vec3, 3, GL_UNSIGNED_BYTE, GL_TRUE );
SPECIALIZE( glm::u8vec4, 4, GL_UNSIGNED_BYTE, GL_TRUE );

#undef SPECIALIZE

//GLAttribBuffer< A0, A1, ... > is a buffer that stores interleaved vertex attribute data of a specific type, and remembers both this type and how many vertices are stored.
// As a convenience, a compatible vertex type is available as GLAttribBuffer< A0, A1, ... >::Vertex.
template< typename... A >
struct GLAttribBuffer;

template< typename A0 >
struct GLAttribBuffer< A0 > : GLBuffer {
	GLsizei count = 0;

	struct Vertex {
		Vertex(A0 &&a0_) : a0(a0_) { }
		Vertex() = default;
		A0 a0;
	};
	static_assert(sizeof(Vertex) == sizeof(A0), "Vertex is packed.");

	GLAttribPointer operator[](uint32_t idx) const {
		assert(idx == 0);
		return GLAttribPointer(buffer,
			GLTypeInfo< A0 >::size,
			GLTypeInfo< A0 >::type,
			GLTypeInfo< A0 >::normalized,
			sizeof(Vertex),
			0
		);
	};

	void set(GLsizei count_, Vertex const *data, GLenum usage) {
		count = count_;
		GLBuffer::set(GL_ARRAY_BUFFER, count_ * sizeof(Vertex), data, usage);
	}
	void set(std::vector< Vertex > const &data, GLenum usage) {
		set(data.size(), &data[0], usage);
	}
};

template< typename A0, typename A1 >
struct GLAttribBuffer< A0, A1 > : GLBuffer {
	GLsizei count = 0;

	struct Vertex {
		Vertex(A0 &&a0_, A1 &&a1_) : a0(std::forward(a0_)), a1(std::forward(a1_)) { }
		Vertex() = default;
		A0 a0;
		A1 a1;
	};
	static_assert(sizeof(Vertex) == sizeof(A0) + sizeof(A1), "Vertex is packed.");

	GLAttribPointer operator[](uint32_t idx) const {
		if (idx == 0) {
			return GLAttribPointer(buffer,
				GLTypeInfo< A0 >::size,
				GLTypeInfo< A0 >::type,
				GLTypeInfo< A0 >::normalized,
				sizeof(Vertex),
				0
			);
		} else if (idx == 1) {
			return GLAttribPointer(buffer,
				GLTypeInfo< A1 >::size,
				GLTypeInfo< A1 >::type,
				GLTypeInfo< A1 >::normalized,
				sizeof(Vertex),
				0 + sizeof(A0)
			);
		} else {
			assert(idx < 2);
			return GLAttribPointer();
		}
	};

	void set(GLsizei count_, Vertex const *data, GLenum usage) {
		count = count_;
		GLBuffer::set(GL_ARRAY_BUFFER, count_ * sizeof(Vertex), data, usage);
	}
	void set(std::vector< Vertex > const &data, GLenum usage) {
		set(data.size(), &data[0], usage);
	}
};

template< typename A0, typename A1, typename A2 >
struct GLAttribBuffer< A0, A1, A2 > : GLBuffer {
	GLsizei count = 0;

	struct Vertex {
		Vertex(A0 &&a0_, A1 &&a1_, A2 &&a2_) : a0(std::forward(a0_)), a1(std::forward(a1_)), a2(std::forward(a2_)) { }
		Vertex() = default;
		A0 a0;
		A1 a1;
		A2 a2;
	};
	static_assert(sizeof(Vertex) == sizeof(A0) + sizeof(A1) + sizeof(A2), "Vertex is packed.");

	GLAttribPointer operator[](uint32_t idx) const {
		if (idx == 0) {
			return GLAttribPointer(buffer,
				GLTypeInfo< A0 >::size,
				GLTypeInfo< A0 >::type,
				GLTypeInfo< A0 >::normalized,
				sizeof(Vertex),
				0
			);
		} else if (idx == 1) {
			return GLAttribPointer(buffer,
				GLTypeInfo< A1 >::size,
				GLTypeInfo< A1 >::type,
				GLTypeInfo< A1 >::normalized,
				sizeof(Vertex),
				0 + sizeof(A0)
			);
		} else if (idx == 2) {
			return GLAttribPointer(buffer,
				GLTypeInfo< A2 >::size,
				GLTypeInfo< A2 >::type,
				GLTypeInfo< A2 >::normalized,
				sizeof(Vertex),
				0 + sizeof(A0) + sizeof(A1)
			);
		} else {
			assert(idx < 2);
			return GLAttribPointer();
		}
	};

	void set(GLsizei count_, Vertex const *data, GLenum usage) {
		count = count_;
		GLBuffer::set(GL_ARRAY_BUFFER, count_ * sizeof(Vertex), data, usage);
	}
	void set(std::vector< Vertex > const &data, GLenum usage) {
		set(data.size(), &data[0], usage);
	}
};

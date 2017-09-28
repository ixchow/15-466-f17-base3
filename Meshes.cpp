#include "Meshes.hpp"
#include "read_chunk.hpp"

#include "GLBuffer.hpp"

#include <glm/glm.hpp>

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <vector>

void Meshes::load(std::string const &filename, Attributes const &attributes) {
	std::ifstream file(filename, std::ios::binary);

	GLuint vao = 0;
	GLuint total = 0;
	{ //read + upload data chunk:
		GLAttribBuffer< glm::vec3, glm::vec3 > buffer;
		std::vector< decltype(buffer)::Vertex > data;
		read_chunk(file, "v3n3", &data);

		//upload data:
		buffer.set(data, GL_STATIC_DRAW);

		total = data.size(); //store total for later checks on index

		glBindBuffer(GL_ARRAY_BUFFER, buffer.buffer);

		//store binding:
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		if (attributes.Position != -1U) {
			glVertexAttribPointer(attributes.Position, 3, GL_FLOAT, GL_FALSE, sizeof(decltype(buffer)::Vertex), (GLbyte *)0);
			glEnableVertexAttribArray(attributes.Position);
		} else {
			std::cerr << "WARNING: loading v3n3 data from '" << filename << "', but not using the Position attribute." << std::endl;
		}
		if (attributes.Normal != -1U) {
			glVertexAttribPointer(attributes.Normal, 3, GL_FLOAT, GL_FALSE, sizeof(decltype(buffer)::Vertex), (GLbyte *)0 + sizeof(glm::vec3));
			glEnableVertexAttribArray(attributes.Normal);
		} else {
			std::cerr << "WARNING: loading v3n3 data from '" << filename << "', but not using the Normal attribute." << std::endl;
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		buffer.buffer = 0; //make it so that when buffer is released, the data is not freed.
	}

	std::vector< char > strings;
	read_chunk(file, "str0", &strings);

	{ //read index chunk, add to meshes:
		struct IndexEntry {
			uint32_t name_begin, name_end;
			uint32_t vertex_start, vertex_count;
		};
		static_assert(sizeof(IndexEntry) == 16, "Index entry should be packed");

		std::vector< IndexEntry > index;
		read_chunk(file, "idx0", &index);

		for (auto const &entry : index) {
			if (!(entry.name_begin <= entry.name_end && entry.name_end <= strings.size())) {
				throw std::runtime_error("index entry has out-of-range name begin/end");
			}
			if (!(entry.vertex_start < entry.vertex_start + entry.vertex_count && entry.vertex_start + entry.vertex_count <= total)) {
				throw std::runtime_error("index entry has out-of-range vertex start/count");
			}
			std::string name(&strings[0] + entry.name_begin, &strings[0] + entry.name_end);
			Mesh mesh;
			mesh.vao = vao;
			mesh.start = entry.vertex_start;
			mesh.count = entry.vertex_count;
			bool inserted = meshes.insert(std::make_pair(name, mesh)).second;
			if (!inserted) {
				std::cerr << "WARNING: mesh name '" + name + "' in filename '" + filename + "' collides with existing mesh." << std::endl;
			}
		}
	}

	if (file.peek() != EOF) {
		std::cerr << "WARNING: trailing data in mesh file '" + filename + "'" << std::endl;
	}
}

Mesh const &Meshes::get(std::string const &name) const {
	auto f = meshes.find(name);
	if (f == meshes.end()) {
		throw std::runtime_error("Looking up mesh that doesn't exist.");
	}
	return f->second;
}

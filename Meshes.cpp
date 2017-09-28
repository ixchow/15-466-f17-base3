#include "Meshes.hpp"
#include "read_chunk.hpp"

#include "GLBuffer.hpp"

#include <glm/glm.hpp>

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <vector>

void Meshes::load(std::string const &filename) {
	std::ifstream file(filename, std::ios::binary);

	GLAttribPointer Position, Normal, Color, TexCoord;

	GLuint total = 0;
	//read + upload data chunk:
	if (filename.size() >= 5 && filename.substr(filename.size()-5) == ".vn") {
		GLAttribBuffer< glm::vec3, glm::vec3 > buffer;
		std::vector< decltype(buffer)::Vertex > data;
		read_chunk(file, "vn00", &data);

		//upload data:
		buffer.set(data, GL_STATIC_DRAW);

		total = data.size(); //store total for later checks on index

		//store attrib locations:
		Position = buffer[0];
		Normal = buffer[1];

		buffer.buffer = 0; //make it so that when buffer is released, the data is not freed.
	} else if (filename.size() >= 7 && filename.substr(filename.size()-7) == ".vnc") {
		GLAttribBuffer< glm::vec3, glm::vec3, glm::u8vec4 > buffer;
		std::vector< decltype(buffer)::Vertex > data;
		read_chunk(file, "vnc0", &data);

		//upload data:
		buffer.set(data, GL_STATIC_DRAW);

		total = data.size(); //store total for later checks on index

		//store attrib locations:
		Position = buffer[0];
		Normal = buffer[1];
		Color = buffer[2];

		buffer.buffer = 0; //make it so that when buffer is released, the data is not freed.
	} else {
		throw std::runtime_error("Unknown file type '" + filename + "'");
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
			mesh.Position = Position;
			mesh.Normal = Normal;
			mesh.Color = Color;
			mesh.TexCoord = TexCoord;
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

#ifndef FONT_H
#define FONT_H
#include <ft2build.h>
#include FT_FREETYPE_H 
#include <glm/glm.hpp>
#include "mesh.hpp"

namespace Font {
	struct Character {
		GLuint texture;
		glm::vec2 uvBottomLeft;
		glm::vec2 uvOffset;
		glm::ivec2 size;
		glm::ivec2 bearing;
		long advance;
	};

	void init();
	void destroy();
	Character getChar(int size, char c);
}
#endif
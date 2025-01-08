#include <ft2build.h>
#include FT_FREETYPE_H 
#include <glm/glm.hpp>
#include "mesh.hpp"

struct Character {
	GLuint texture;
	glm::vec2 uvBottomLeft;
	glm::ivec2 size;
	glm::ivec2 bearing;
	long advance;
};

void initFont();
Character getChar(int size, char c);
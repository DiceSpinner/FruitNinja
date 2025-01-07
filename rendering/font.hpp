#include <ft2build.h>
#include FT_FREETYPE_H 
#include <glm/glm.hpp>
#include "mesh.hpp"

struct Character {
	GLuint texture;
	glm::vec2 uv;
	glm::ivec2 size;
	glm::ivec2 bearing;
	unsigned int advance;
};

void initFont();
void loadFont(int size);
Character getChar(int size, char c);
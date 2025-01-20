#include <ft2build.h>
#include FT_FREETYPE_H 
#include <glm/glm.hpp>
#include "mesh.hpp"

struct Character {
	GLuint texture;
	glm::vec2 uvBottomLeft;
	glm::vec2 uvOffset;
	glm::ivec2 size;
	glm::ivec2 bearing;
	long advance;
};

void initFont();
void deinitFont();
Character getChar(int size, char c);
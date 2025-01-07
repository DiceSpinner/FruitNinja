#include <iostream>
#include <unordered_map>
#include <utility>
#include <glad/glad.h>
#include "font.hpp"

using namespace std;

template <typename T1, typename T2>
struct pair_hash {
    size_t operator()(const std::pair<T1, T2>& p) const {
        // Combine the hashes of the two elements using XOR and bit-shifting
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 << 1);  // Combine the two hash values
    }
};

static const char* fontPath = "../fonts/history_of_wawa/History of Wawa.ttf";
static FT_Library lib;
static FT_Face face;
static unordered_map<pair<int, char>, Character, pair_hash<int, char>> loadedCharacters;

void initFont() {
	if (FT_Init_FreeType(&lib)) {
		cout << "Failed to initialize free type library" << std::endl;
		exit(1);
	}

	if (FT_New_Face(lib, fontPath, 0, &face))
	{
		std::cout << " Failed to load font" << std::endl;
		exit(1);
	}
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void loadFont() {
    
}

Character getChar(int size, char c) {
	auto font = loadedCharacters.find({size, c});
	if (font != loadedCharacters.end()) {
		return font->second;
	}

	FT_Set_Pixel_Sizes(face, 0, size);
	if (FT_Load_Char(face, c, FT_LOAD_RENDER))
	{
		std::cout << "Failed to load Glyph for char " << c << std::endl;
		exit(1);
	}
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        face->glyph->bitmap.width,
        face->glyph->bitmap.rows,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        face->glyph->bitmap.buffer
    );
    // set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // now store character for later use
    Character character = {
        texture,
        glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
        glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
        glm::ivec2(),
        face->glyph->advance.x
    };
    loadedCharacters.insert({ {size, c }, character});
}
#include <iostream>
#include <unordered_map>
#include <utility>
#include <glad/glad.h>
#include "font.hpp"

using namespace std;
using namespace Font;

static const char* fontPath = FONT_DIR "history_of_wawa/History of Wawa.ttf";
static FT_Library lib;
static FT_Face face;
static unordered_map<int, unordered_map<char, Character>> loadedFonts;

void Font::init() {
	if (FT_Init_FreeType(&lib)) {
		cout << "Failed to initialize free type library" << std::endl;
		exit(1);
	}

    auto i = FT_New_Face(lib, fontPath, 0, &face);
	if (i)
	{
		std::cout << " Failed to load font" << std::endl;
		exit(1);
	}
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void Font::destroy() {
    FT_Done_Face(face);
    FT_Done_FreeType(lib);
}

Character Font::getChar(int size, char c) {
	auto font = loadedFonts.find(size);
	if (font != loadedFonts.end()) {
		return font->second[c];
	}
    unordered_map<char, Character> characters;

    unsigned int atlas;
    glGenTextures(1, &atlas);
    glBindTexture(GL_TEXTURE_2D, atlas);
    int atlasWidth = 1024;
    int atlasHeight = 1024;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R8,
        atlasWidth,
        atlasHeight,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    FT_Set_Pixel_Sizes(face, 0, size);

    int offsetX = 0;
    int offsetY = 0;
    int maxY = 0;
    for (unsigned char i = 0; i <= 200;i++) {
        if (FT_Load_Char(face, i, FT_LOAD_RENDER))
        {
            std::cout << "Failed to load Glyph for char " << i << std::endl;
            continue;
        }
        if (face->glyph->bitmap.rows > maxY) {
            maxY = face->glyph->bitmap.rows;
        }

        unsigned char* buffer = new unsigned char[face->glyph->bitmap.rows * face->glyph->bitmap.width];

        for (auto k = 0; k < face->glyph->bitmap.rows;k++) {
            memcpy(
                &(buffer[(face->glyph->bitmap.rows - 1 - k) * face->glyph->bitmap.width]), 
                &(face->glyph->bitmap.buffer[k * face->glyph->bitmap.width]), 
                face->glyph->bitmap.width
            );
        }

        if (face->glyph->bitmap.width + offsetX >= atlasWidth) {
            offsetX = 0;
            offsetY += maxY;
            maxY = 0;
        }

        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            offsetX,
            offsetY,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            GL_RED,
            GL_UNSIGNED_BYTE,
            buffer
        );
        delete[] buffer;

        glm::vec2 uv(offsetX / (float)atlasWidth, offsetY / (float)atlasHeight);
        glm::vec2 offset(face->glyph->bitmap.width / (float)atlasWidth, face->glyph->bitmap.rows / (float)atlasHeight);
        uv.x += 0.5 / atlasWidth;
        uv.y += 0.5 / atlasHeight;
        offset.x -= 1 / (float)atlasWidth;
        offset.y -= 1 / (float)atlasHeight;

        characters.emplace(i,
            Character{
                atlas,
                uv,
                offset,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                face->glyph->advance.x >> 6
            }
        );
        offsetX += max((unsigned int)(face->glyph->advance.x >> 6), face->glyph->bitmap.width);
    }
    loadedFonts[size] = characters;
    return characters[c];
}
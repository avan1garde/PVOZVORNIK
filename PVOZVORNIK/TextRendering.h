#ifndef TEXT_RENDERING_H
#define TEXT_RENDERING_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <map>
#include <string>

struct Character {
    unsigned int TextureID;
    glm::ivec2 Size; 
    glm::ivec2 Bearing; 
    unsigned int Advance; 
};

void loadFont(const char* fontPath);
void initTextRendering();
void renderText(unsigned int shader, const std::string& text, float x, float y, float scale, glm::vec3 color);

#endif // TEXT_RENDERING_H


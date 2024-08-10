#include "image_helpers.h"

#include <GL/gl3w.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

ImageTexture LoadTextureFromFile(const char* file_name)
{
    int image_width, image_height;
    unsigned char* image_data = stbi_load(file_name, &image_width, &image_height, NULL, 4);

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    return ImageTexture{(void*) (intptr_t) image_texture, image_width, image_height};
}

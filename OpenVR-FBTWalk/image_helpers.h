#pragma once

struct ImageTexture {
    void* id = nullptr;
    int width;
    int height;
};

ImageTexture LoadTextureFromFile(const char* file_name);

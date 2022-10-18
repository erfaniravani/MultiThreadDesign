#include <vector>

#pragma pack(1)
#pragma once

#define FILE_HEADER_SIZE 14
#define INFO_HEADER_SIZE 124

struct Color{
    int r, g, b;

    Color();
    Color(int r, int g, int b);
    ~Color();
};

struct Image{
    int rows, cols;
    std::vector<Color> colors;

    Image();
    Image(int rows, int cols);
    ~Image();
};
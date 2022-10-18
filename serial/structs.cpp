#include "structs.hpp"

Color::Color() : r(0), g(0), b(0){}

Color::Color(int r, int g, int b) : r(r), g(g), b(b){}

Color::~Color(){}

Image::Image() : rows(0), cols(0){}

Image::Image(int rows, int cols) : rows(rows), cols(cols), colors(std::vector<Color>(cols*rows)) {}

Image::~Image(){}

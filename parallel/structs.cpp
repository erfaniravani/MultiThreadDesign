#include "structs.hpp"

Color::Color() : r(0), g(0), b(0){}

Color::Color(int r, int g, int b) : r(r), g(g), b(b){}

Color::~Color(){}

Image::Image() : rows(0), cols(0){}

Image::Image(int rows, int cols) : rows(rows), cols(cols), colors(std::vector<Color>(cols*rows)) {}

Image::~Image(){}

Thread_data::Thread_data() : thread_number(0){}

Thread_data::Thread_data(int thread_number, Image *image) : thread_number(thread_number), image(image){}

Thread_data::~Thread_data(){}

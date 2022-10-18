#include <iostream>
#include <unistd.h>
#include <fstream>
#include <pthread.h>
#include "structs.hpp"
#include <sys/time.h>

using namespace std;

bool read_pic(const char* filename, Image &image){
    ifstream pic;
    pic.open(filename, ios::out | ios::binary);

    if(!pic.is_open()){
        cout << "Could not open the file to read" << endl;
        return false;
    }

    unsigned char pic_header[FILE_HEADER_SIZE];
    unsigned char pic_info[INFO_HEADER_SIZE];

    pic.read(reinterpret_cast<char*>(pic_header), FILE_HEADER_SIZE);
    pic.read(reinterpret_cast<char*>(pic_info), INFO_HEADER_SIZE);

    int pic_size = pic_header[2] + (pic_header[3] << 8) + (pic_header[4] << 16) + (pic_header[5] << 24);
    int cols = pic_info[4] + (pic_info[5] << 8) + (pic_info[6] << 16) + (pic_info[7] << 24);
    int rows = pic_info[8] + (pic_info[9] << 8) + (pic_info[10] << 16) + (pic_info[11] << 24);

    Image picture(rows, cols);
    const int padding = (4 - (cols * 3) %4 ) %4;

    for (int i=0; i<rows; i++){
        for (int j=0; j<cols; j++){
            unsigned char color[3];
            pic.read(reinterpret_cast<char*>(color), 3);

            picture.colors[i*cols + j].r = static_cast<int>(color[0]);            
            picture.colors[i*cols + j].g = static_cast<int>(color[1]);
            picture.colors[i*cols + j].b = static_cast<int>(color[2]);
        }
        pic.ignore(padding);
    }
    pic.close();
    image = picture;
    return true;
}
bool write_pic(const char* filename, Image image){
    ofstream pic;
    pic.open(filename, ios::out | ios::binary);

    if(!pic.is_open()){
        cout << "Could not open the file to write" << endl;
        return false;
    }

    unsigned char bmp_pad[3] = {0, 0, 0};
    const int padding = (4 - (image.cols * 3) %4 ) %4;
    const int filesize = FILE_HEADER_SIZE + INFO_HEADER_SIZE + 3 * image.cols * image.rows + padding * image.rows;

    unsigned char pic_header[FILE_HEADER_SIZE] = {0};
    pic_header[0] = 'B';
    pic_header[1] = 'M';
    pic_header[2] = filesize;
    pic_header[3] = filesize >> 8; 
    pic_header[4] = filesize >> 16;
    pic_header[5] = filesize >> 24;
    pic_header[10] = FILE_HEADER_SIZE + INFO_HEADER_SIZE;

    unsigned char info_header[INFO_HEADER_SIZE] = {0};
    info_header[0] = INFO_HEADER_SIZE;
    info_header[4] = image.cols;
    info_header[5] = image.cols >> 8;
    info_header[6] = image.cols >> 16;
    info_header[7] = image.cols >> 24;
    info_header[8] = image.rows;
    info_header[9] = image.rows >> 8;
    info_header[10] = image.rows >> 16;
    info_header[11] = image.rows >> 24;
    info_header[12] = 1;
    info_header[14] = 24;

    pic.write(reinterpret_cast<char*>(pic_header), FILE_HEADER_SIZE);
    pic.write(reinterpret_cast<char*>(info_header), INFO_HEADER_SIZE);

    for (int i=0; i<image.rows; i++){
        for (int j=0; j<image.cols; j++){

            unsigned char r = static_cast<unsigned char>(image.colors[i*image.cols + j].r);            
            unsigned char g = static_cast<unsigned char>(image.colors[i*image.cols + j].g);
            unsigned char b = static_cast<unsigned char>(image.colors[i*image.cols + j].b);

            unsigned char tmp[3] = {r, g, b};
            pic.write(reinterpret_cast<char*>(tmp), 3);
        }
        pic.write(reinterpret_cast<char*>(bmp_pad), padding);
    }
    pic.close();
    return true;
}
void filter_one(Image &picture){
    int rows = picture.rows;
    int cols = picture.cols;
    int pivot;
    if(cols % 2 == 0)
        pivot = (cols + 1) /2;
    else
        pivot = cols / 2;

    for (int i = 0 ; i < rows ; i++){
        for (int j = 0 ; j < cols ; j++){
            if(j < pivot){
                struct Color temp_color;
                temp_color = picture.colors[i*cols + j];
                picture.colors[i*cols + j] = picture.colors[i*cols + cols - j - 1];
                picture.colors[i*cols + cols - j - 1] = temp_color;
            }
            
        }
    }
}
void filter_two(Image &picture){
    int rows = picture.rows;
    int cols = picture.cols;
    int pivot;
    if(rows % 2 == 0)
        pivot = (rows + 1) /2;
    else
        pivot = rows / 2;

    for (int i = 0 ; i < rows ; i++){
        for (int j = 0 ; j < cols ; j++){
            if((i*cols + j) < pivot*cols){
                struct Color temp_color;
                temp_color = picture.colors[i*cols + j];
                picture.colors[i*cols + j] = picture.colors[(rows - i - 1)*cols +  j];
                picture.colors[(rows - i - 1)*cols +  j] = temp_color;
            }
        }
    }
}
void filter_three(Image &picture){
    int rows = picture.rows;
    int cols = picture.cols;

    for (int i = 1 ; i < rows-1 ; i++){
        for (int j = 1 ; j < cols-1 ; j++){
            int array_red[9];
            array_red[0] = (picture.colors[(i-1)*cols + j].r);//up
            array_red[1] = (picture.colors[(i-1)*cols + j + 1].r);//up right
            array_red[2] = (picture.colors[(i-1)*cols + j - 1].r);//up left
            array_red[3] = (picture.colors[i*cols + j].r);//itself
            array_red[4] = (picture.colors[i*cols + j + 1].r);//itself right
            array_red[5] = (picture.colors[i*cols + j - 1].r);//itself left
            array_red[6] = (picture.colors[(i+1)*cols + j].r);//down
            array_red[7] = (picture.colors[(i+1)*cols + j + 1].r);//down right
            array_red[8] = (picture.colors[(i+1)*cols + j - 1].r);//down left
            sort(array_red, array_red + 9);
            picture.colors[i*cols + j].r = array_red[4];

            int array_blue[9];
            array_blue[0] = (picture.colors[(i-1)*cols + j].b);//up
            array_blue[1] = (picture.colors[(i-1)*cols + j + 1].b);//up right
            array_blue[2] = (picture.colors[(i-1)*cols + j - 1].b);//up left
            array_blue[3] = (picture.colors[i*cols + j].b);//itself
            array_blue[4] = (picture.colors[i*cols + j + 1].b);//itself right
            array_blue[5] = (picture.colors[i*cols + j - 1].b);//itself left
            array_blue[6] = (picture.colors[(i+1)*cols + j].b);//down
            array_blue[7] = (picture.colors[(i+1)*cols + j + 1].b);//down right
            array_blue[8] = (picture.colors[(i+1)*cols + j - 1].b);//down left
            sort(array_blue, array_blue + 9);
            picture.colors[i*cols + j].b = array_blue[4];

            int array_green[9];
            array_green[0] = (picture.colors[(i-1)*cols + j].g);//up
            array_green[1] = (picture.colors[(i-1)*cols + j + 1].g);//up right
            array_green[2] = (picture.colors[(i-1)*cols + j - 1].g);//up left
            array_green[3] = (picture.colors[i*cols + j].g);//itself
            array_green[4] = (picture.colors[i*cols + j + 1].g);//itself right
            array_green[5] = (picture.colors[i*cols + j - 1].g);//itself left
            array_green[6] = (picture.colors[(i+1)*cols + j].g);//down
            array_green[7] = (picture.colors[(i+1)*cols + j + 1].g);//down right
            array_green[8] = (picture.colors[(i+1)*cols + j - 1].g);//down left
            sort(array_green, array_green + 9);
            picture.colors[i*cols + j].g = array_green[4];
        }
    }
}
void filter_four(Image &picture){
    int rows = picture.rows;
    int cols = picture.cols;

    for (int i=0; i<rows; i++){
        for (int j=0; j<cols; j++){
            picture.colors[i*cols + j].r = 255 - picture.colors[i*cols + j].r;
            picture.colors[i*cols + j].g = 255 - picture.colors[i*cols + j].g;
            picture.colors[i*cols + j].b = 255 - picture.colors[i*cols + j].b;
        }
    }
}
void filter_five(Image &picture){

    int rows = picture.rows;
    int cols = picture.cols;
    int col_index = cols / 2;
    int row_index = rows / 2;
    
    // vertical
    for(int k = 0 ; k < rows ; k++){
        picture.colors[k*cols + col_index].r = 255;
        picture.colors[k*cols + col_index].b = 255;
        picture.colors[k*cols + col_index].g = 255;

        picture.colors[k*cols + col_index - 1].r = 255;
        picture.colors[k*cols + col_index - 1].b = 255;
        picture.colors[k*cols + col_index - 1].g = 255;

        picture.colors[k*cols + col_index + 1].r = 255;
        picture.colors[k*cols + col_index + 1].b = 255;
        picture.colors[k*cols + col_index + 1].g = 255;
    }
    //horizental
    for(int t = 0 ; t < cols ; t++){
        picture.colors[row_index*cols + t].r = 255;
        picture.colors[row_index*cols + t].b = 255;
        picture.colors[row_index*cols + t].g = 255;

        picture.colors[(row_index-1)*cols + t].r = 255;
        picture.colors[(row_index-1)*cols + t].b = 255;
        picture.colors[(row_index-1)*cols + t].g = 255;

        picture.colors[(row_index+1)*cols + t].r = 255;
        picture.colors[(row_index+1)*cols + t].b = 255;
        picture.colors[(row_index+1)*cols + t].g = 255;
    }
}

void filter_edge(Image &picture){
    int rows = picture.rows;
    int cols = picture.cols;

    for (int i = 1 ; i < rows-1 ; i++){
        for (int j = 1 ; j < cols-1 ; j++){
            int array_red[9];
            array_red[0] = (picture.colors[(i-1)*cols + j].r);//up
            array_red[1] = (picture.colors[(i-1)*cols + j + 1].r);//up right
            array_red[2] = (picture.colors[(i-1)*cols + j - 1].r);//up left
            array_red[3] = (picture.colors[i*cols + j].r);//itself
            array_red[4] = (picture.colors[i*cols + j + 1].r);//itself right
            array_red[5] = (picture.colors[i*cols + j - 1].r);//itself left
            array_red[6] = (picture.colors[(i+1)*cols + j].r);//down
            array_red[7] = (picture.colors[(i+1)*cols + j + 1].r);//down right
            array_red[8] = (picture.colors[(i+1)*cols + j - 1].r);//down left
            int edge_val = array_red[0]*-1 + array_red[1]*-1 + array_red[2]*-1 + array_red[3]*8 + array_red[4]*-1 + array_red[5]*-1
                            + array_red[6]*-1 + array_red[7]*-1 + array_red[8]*-1;
            picture.colors[i*cols + j].r = edge_val;

            int array_blue[9];
            array_blue[0] = (picture.colors[(i-1)*cols + j].b);//up
            array_blue[1] = (picture.colors[(i-1)*cols + j + 1].b);//up right
            array_blue[2] = (picture.colors[(i-1)*cols + j - 1].b);//up left
            array_blue[3] = (picture.colors[i*cols + j].b);//itself
            array_blue[4] = (picture.colors[i*cols + j + 1].b);//itself right
            array_blue[5] = (picture.colors[i*cols + j - 1].b);//itself left
            array_blue[6] = (picture.colors[(i+1)*cols + j].b);//down
            array_blue[7] = (picture.colors[(i+1)*cols + j + 1].b);//down right
            array_blue[8] = (picture.colors[(i+1)*cols + j - 1].b);//down left
            edge_val = array_blue[0]*-1 + array_blue[1]*-1 + array_blue[2]*-1 + array_blue[3]*8 + array_blue[4]*-1 + array_blue[5]*-1
                            + array_blue[6]*-1 + array_blue[7]*-1 + array_blue[8]*-1;
            picture.colors[i*cols + j].b = edge_val;

            int array_green[9];
            array_green[0] = (picture.colors[(i-1)*cols + j].g);//up
            array_green[1] = (picture.colors[(i-1)*cols + j + 1].g);//up right
            array_green[2] = (picture.colors[(i-1)*cols + j - 1].g);//up left
            array_green[3] = (picture.colors[i*cols + j].g);//itself
            array_green[4] = (picture.colors[i*cols + j + 1].g);//itself right
            array_green[5] = (picture.colors[i*cols + j - 1].g);//itself left
            array_green[6] = (picture.colors[(i+1)*cols + j].g);//down
            array_green[7] = (picture.colors[(i+1)*cols + j + 1].g);//down right
            array_green[8] = (picture.colors[(i+1)*cols + j - 1].g);//down left
            edge_val = array_green[0]*-1 + array_green[1]*-1 + array_green[2]*-1 + array_green[3]*8 + array_green[4]*-1 + array_green[5]*-1
                            + array_green[6]*-1 + array_green[7]*-1 + array_green[8]*-1;
            picture.colors[i*cols + j].g = edge_val;
        }
    }
}

int main(int argc, char *argv[]){
    char *fileBuffer;
    int bufferSize;

    if(argc < 2){
        cout << "no fileName entered" << endl;
        return 0;
    }

    Image picture;

    // read input file
    if (!read_pic(argv[1], picture))
        cout << "failed to read picture" << endl;
    else cout << "picture read was successful" << endl;
    cout << "**************************************************" << endl;
    cout << "Pic Data :\n" << "rows = " << picture.rows << " ,cols = " << picture.cols << endl;
    cout << "**************************************************" << endl;

    // apply filters
    clock_t start1, end1, start2, end2, start3, end3, start4, end4, start5, end5;
    //(x,y)--->(-x,y)
    start1 = clock();
    filter_one(picture);
    end1 = clock();
    int clock_taken = end1 - start1;
    cout << "time of part 1 : " << fixed << clock_taken << endl;
    //(x,y)--->(x,-y)
    start2 = clock();
    filter_two(picture); 
    end2 = clock();
    clock_taken = end2 - start2;
    cout << "time of part 2 : " << fixed << clock_taken << endl; 
    //median
    start3 = clock();
    filter_three(picture);
    end3 = clock();
    clock_taken = end3 - start3;
    cout << "time of part 3 : " << fixed << clock_taken << endl; 
    //color diff
    // start4 = clock();
    // filter_four(picture);
    // end4 = clock();
    // clock_taken = end4 - start4;
    // cout << "time of part 4 : " << fixed << clock_taken << endl; 
    // //plus sign
    // start5 = clock();
    // filter_five(picture);
    // end5 = clock();
    // clock_taken = end5 - start5;
    // cout << "time of part 5 : " << fixed << clock_taken << endl; 
    //edge
    start4 = clock();
    filter_edge(picture);
    end4 = clock();
    clock_taken = end4 - start4;
    cout << "time of part 4 : " << fixed << clock_taken << endl; 
    // write output file
    if (!write_pic("output.bmp", picture))
        cout << "failed to write picture" << endl;
    else cout << "picture write was successful" << endl;

    return 0;
}
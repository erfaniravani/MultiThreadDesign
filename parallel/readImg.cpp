#include <iostream>
#include <unistd.h>
#include <fstream>
#include <pthread.h>
#include "structs.hpp"

#define NUM_THREADS 4
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
void *filter_one_thread(void *arg){
    struct Thread_data *t_data = (Thread_data*) arg;
    struct Image *picture = t_data->image;

    int rows = picture->rows;
    int cols = picture->cols;
    int start_index = t_data->thread_number*rows/NUM_THREADS;
    int end_index = (t_data->thread_number+1)*rows/NUM_THREADS;
    int pivot = cols/2;
    for (int i = start_index ; i < end_index ; i++){
        for (int j = 0 ; j < cols ; j++){
            if(j < pivot){
                struct Color temp_color;
                temp_color = picture->colors[i*cols + j];
                picture->colors[i*cols + j] = picture->colors[i*cols + cols - j - 1];
                picture->colors[i*cols + cols - j - 1] = temp_color;
            }
            else
                break;
        }
    }
    pthread_exit(NULL);
}
void *filter_two_thread(void *arg){
    struct Thread_data *t_data = (Thread_data*) arg;
    struct Image *picture = t_data->image;

    int rows = picture->rows;
    int cols = picture->cols;
    int start_index = t_data->thread_number*cols/NUM_THREADS;
    int end_index = (t_data->thread_number+1)*cols/NUM_THREADS;
    int pivot = rows/2;

    for (int i = 0 ; i < rows ; i++){
        for (int j = start_index ; j < end_index ; j++){

            if(i < pivot){
                struct Color temp_color;
                temp_color = picture->colors[i*cols + j];
                picture->colors[i*cols + j] = picture->colors[(rows - i - 1)*cols +  j];
                picture->colors[(rows - i - 1)*cols +  j] = temp_color;
            }
            else
                break;
        }
    }
    return picture;
}
void *filter_three_thread(void *arg){
    struct Thread_data *t_data = (Thread_data*) arg;
    struct Image *picture = t_data->image;

    int rows = picture->rows;
    int cols = picture->cols;
    int all_pixels = cols * rows;
    int start_index = t_data->thread_number*all_pixels/NUM_THREADS;
    int end_index = (t_data->thread_number+1)*all_pixels/NUM_THREADS;
    for(int i = start_index ; i < end_index ; i++){
        if(i < cols)
            continue;
        if(i >= cols*(rows-1))
            continue;
        if(i%cols == 0)
            continue;
        if(i%cols == cols-1)
            continue;
        int array_red[9];
        array_red[0] = (picture->colors[i - cols].r);//up
        array_red[1] = (picture->colors[i - cols + 1].r);//up right
        array_red[2] = (picture->colors[i - cols - 1].r);//up left
        array_red[3] = (picture->colors[i].r);//itself
        array_red[4] = (picture->colors[i + 1].r);//itself right
        array_red[5] = (picture->colors[i - 1].r);//itself left
        array_red[6] = (picture->colors[i + cols].r);//down
        array_red[7] = (picture->colors[i + cols + 1].r);//down right
        array_red[8] = (picture->colors[i + cols - 1].r);//down left
        sort(array_red, array_red + 9);
        picture->colors[i].r = array_red[4];

        int array_blue[9];
        array_blue[0] = (picture->colors[i - cols].b);//up
        array_blue[1] = (picture->colors[i - cols + 1].b);//up right
        array_blue[2] = (picture->colors[i - cols - 1].b);//up left
        array_blue[3] = (picture->colors[i].b);//itself
        array_blue[4] = (picture->colors[i + 1].b);//itself right
        array_blue[5] = (picture->colors[i - 1].b);//itself left
        array_blue[6] = (picture->colors[i + cols].b);//down
        array_blue[7] = (picture->colors[i + cols + 1].b);//down right
        array_blue[8] = (picture->colors[i + cols - 1].b);//down left
        sort(array_blue, array_blue + 9);
        picture->colors[i].b = array_blue[4];

        int array_green[9];
        array_green[0] = (picture->colors[i - cols].g);//up
        array_green[1] = (picture->colors[i - cols + 1].g);//up right
        array_green[2] = (picture->colors[i - cols - 1].g);//up left
        array_green[3] = (picture->colors[i].g);//itself
        array_green[4] = (picture->colors[i + 1].g);//itself right
        array_green[5] = (picture->colors[i - 1].g);//itself left
        array_green[6] = (picture->colors[i + cols].g);//down
        array_green[7] = (picture->colors[i + cols + 1].g);//down right
        array_green[8] = (picture->colors[i + cols - 1].g);//down left
        sort(array_green, array_green + 9);
        picture->colors[i].g = array_green[4];

    }
    pthread_exit(NULL);
}
void *filter_four_thread(void *arg){
    struct Thread_data *t_data = (Thread_data*) arg;
    struct Image *picture = t_data->image;

    int rows = picture->rows;
    int cols = picture->cols;
    int all_pixels = cols * rows;
    int start_index = t_data->thread_number*all_pixels/NUM_THREADS;
    int end_index = (t_data->thread_number+1)*all_pixels/NUM_THREADS;
    for(int i = start_index ; i < end_index ; i++){
        picture->colors[i].r = 255 - picture->colors[i].r;
        picture->colors[i].g = 255 - picture->colors[i].g;
        picture->colors[i].b = 255 - picture->colors[i].b;
    }
    return picture;
}
void *filter_five_thread(void *arg){
    struct Thread_data *t_data = (Thread_data*) arg;
    struct Image *picture = t_data->image;

    int rows = picture->rows;
    int cols = picture->cols;
    int col_index = cols / 2;
    int row_index = rows / 2;
    // vertical
    int start_index = t_data->thread_number*rows/NUM_THREADS;
    int end_index = (t_data->thread_number+1)*rows/NUM_THREADS;
    for(int k = start_index ; k < end_index ; k++){
        picture->colors[k*cols + col_index].r = 255;
        picture->colors[k*cols + col_index].b = 255;
        picture->colors[k*cols + col_index].g = 255;

        picture->colors[k*cols + col_index - 1].r = 255;
        picture->colors[k*cols + col_index - 1].b = 255;
        picture->colors[k*cols + col_index - 1].g = 255;

        picture->colors[k*cols + col_index + 1].r = 255;
        picture->colors[k*cols + col_index + 1].b = 255;
        picture->colors[k*cols + col_index + 1].g = 255;
    }
    //horizental
    start_index = t_data->thread_number*cols/NUM_THREADS;
    end_index = (t_data->thread_number+1)*cols/NUM_THREADS;
    for(int t = start_index ; t < end_index ; t++){
        picture->colors[row_index*cols + t].r = 255;
        picture->colors[row_index*cols + t].b = 255;
        picture->colors[row_index*cols + t].g = 255;

        picture->colors[(row_index-1)*cols + t].r = 255;
        picture->colors[(row_index-1)*cols + t].b = 255;
        picture->colors[(row_index-1)*cols + t].g = 255;

        picture->colors[(row_index+1)*cols + t].r = 255;
        picture->colors[(row_index+1)*cols + t].b = 255;
        picture->colors[(row_index+1)*cols + t].g = 255;
    }
    return picture;
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
    int total_pixels = picture.rows*picture.cols;
    clock_t start, end;
    pthread_t th[NUM_THREADS];
    std::vector<Thread_data> t_param;
    // initial inputs for each thread
    for(int i = 0 ; i < NUM_THREADS ; i++){
        Thread_data td(i, &picture);
        t_param.push_back(td);
    }
    start = clock();
    // creat threads 1
    for(int i = 0 ; i < NUM_THREADS ; i++){
        pthread_create(&th[i], NULL, filter_one_thread, &t_param[i]);
    }
    // wait for threads 1
    for(int i = 0 ; i < NUM_THREADS ; i++){
        pthread_join(th[i], NULL);
    }
    // creat threads 2
    for(int i = 0 ; i < NUM_THREADS ; i++){
        pthread_create(&th[i], NULL, filter_two_thread, &t_param[i]);
    }
    // wait for threads 2
    for(int i = 0 ; i < NUM_THREADS ; i++){
        pthread_join(th[i], NULL);
    }
    // creat threads 3
    for(int i = 0 ; i < NUM_THREADS ; i++){
        pthread_create(&th[i], NULL, filter_three_thread, &t_param[i]);
    }
    // wait for threads 3
    for(int i = 0 ; i < NUM_THREADS ; i++){
        pthread_join(th[i], NULL);
    }
    // creat threads 4
    for(int i = 0 ; i < NUM_THREADS ; i++){
        pthread_create(&th[i], NULL, filter_four_thread, &t_param[i]);
    }
    // wait for threads 4
    for(int i = 0 ; i < NUM_THREADS ; i++){
        pthread_join(th[i], NULL);
    }
    // creat threads 5
    for(int i = 0 ; i < NUM_THREADS ; i++){
        pthread_create(&th[i], NULL, filter_five_thread, &t_param[i]);
    }
    // wait for threads 5
    for(int i = 0 ; i < NUM_THREADS ; i++){
        pthread_join(th[i], NULL);
    }
    end = clock();
    int clock_taken = end - start;
    cout << "time taken in clock : " << fixed << clock_taken << endl;

    // write output file
    if (!write_pic("output.bmp", picture))
        cout << "failed to write picture" << endl;
    else cout << "picture write was successful" << endl;


    return 0;
}
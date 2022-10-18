CC := g++ -std=c++11 -Ipthread

all: ImageFilters.out

ImageFilters.out: readImg.o structs.o
	$(CC) readImg.o structs.o -o ImageFilters.out

readImg.o: readImg.cpp structs.hpp
	$(CC) -c readImg.cpp -o readImg.o

structs.o: structs.hpp
	$(CC) -c structs.cpp -o structs.o

clean:
	rm *.o
	rm ImageFilters.out
	rm output.bmp
CC = g++
CFLAGS = -Wall -g

#main: main.o Point.o Square.o
#    $(CC) $(CFLAGS) -o main main.o Point.o Square.o
main: main.o cuckoo_hash.o
	$(CC) $(CFLAGS) -o main main.o -std=gnu++0x

#main.o: main.cpp Point.h Square.h
#    $(CC) $(CFLAGS) -c main.cpp
main.o: main.cpp cuckoo_hash.h
	$(CC) $(CFLAGS) -c main.cpp -std=gnu++0x

cuckoo_hash.o: cuckoo_hash.h
#Point.o: Point.hSquare.o: Square.h Point.h

#Square.o: Square.h Point.h

clean:
	rm -rf *.o
	rm main

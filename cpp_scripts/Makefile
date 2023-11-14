CC = g++
CFLAGS = -Wall -g

#main: main.o Point.o Square.o
#    $(CC) $(CFLAGS) -o main main.o Point.o Square.o
main: main.o cuckoo_hash.o
	$(CC) $(CFLAGS) -o main main.o cuckoo_hash.o -std=gnu++0x

#main.o: main.cpp Point.h Square.h
#    $(CC) $(CFLAGS) -c main.cpp
main.o: main.cpp cuckoo_hash.h cuckoo_hash.cpp
	$(CC) $(CFLAGS) -c main.cpp -std=gnu++0x

cuckoo_hash.o: cuckoo_hash.h cuckoo_hash.cpp
	$(CC) $(CFLAGS) -c cuckoo_hash.cpp -std=gnu++0x
#Point.o: Point.hSquare.o: Square.h Point.h

#Square.o: Square.h Point.h
check: 
	./main example_table_1.txt 5 6 example_table_2.txt 4 6 bruh bruh bruh 111

clean:
	rm -rf *.o
	rm main

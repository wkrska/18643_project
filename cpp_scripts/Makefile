CC = g++
CFLAGS = -Wall -g

main: main.o cuckoo_hash.o hash_functions.o
	$(CC) $(CFLAGS) -o main main.o cuckoo_hash.o hash_functions.o -std=gnu++0x

main.o: main.cpp cuckoo_hash.h cuckoo_hash.cpp hash_functions.h hash_functions.cpp
	$(CC) $(CFLAGS) -c main.cpp -std=gnu++0x

cuckoo_hash.o: cuckoo_hash.h cuckoo_hash.cpp
	$(CC) $(CFLAGS) -c cuckoo_hash.cpp -std=gnu++0x

hash_functions.o: hash_functions.h hash_functions.cpp
	$(CC) $(CFLAGS) -c hash_functions.cpp -std=gnu++0x

check: 
	./main example_table_1.txt 5 15 example_table_2.txt 4 14 bruh bruh bruh 4

clean:
	rm -rf *.o
	rm main

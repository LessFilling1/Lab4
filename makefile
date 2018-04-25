CC=gcc
CFLAGS=-I.

create_bin : 
	mkdir -p obj bin

utils.o : utils.h
	$(CC) -o obj/utils.o -c utils.c $(CFLAGS)

find_min_max.o : utils.h find_min_max.h
	$(CC) -o obj/find_min_max.o -c find_min_max.c $(CFLAGS)


parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o bin/parallel_min_max obj/utils.o obj/find_min_max.o parallel_min_max.c $(CFLAGS)

process_memory : process_memory.c 
	$(CC) -g -o bin/process_memory process_memory.c 

zombie :
	$(CC) -o bin/zombie zombie.c

parallel_sum : utils.o utils.h 
	$(CC) -g -pthread -o bin/parallel_sum obj/utils.o parallel_sum.c $(CFLAGS)

clean :
	rm -rf bin obj

exec_min_max : parallel_min_max
	$(CC) -o bin/exec_min_max exec_min_max.c $(CFLAGS)

all : create_bin zombie parallel_min_max exec_min_max  parallel_sum
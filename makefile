CC=gcc -O3
FLAGS=-lm

jumper: jumper.o heap.o record.o
	$(CC) -o $@ $^ $(FLAGS)

jumper.o: jumper.c
	$(CC) -c $^

heap.o: heap.c
	$(CC) -c $^

record.o: record.c
	$(CC) -c $^

clean:
	rm -rf *.o

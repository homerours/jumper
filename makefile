CC=gcc -O3
FLAGS=-lm

jumper: jumper.o heap.o
	$(CC) -o $@ $^ $(FLAGS)

jumper.o: jumper.c
	$(CC) -c $^

heap.o: heap.c
	$(CC) -c $^

clean:
	rm -rf *.o

CC=gcc -O3
FLAGS=-lm

jumper: jumper.o heap.o
	$(CC) -o $@ $^ $(FLAGS)

jumper.o: jumper.c
	$(CC) -c $^ $(FLAGS)

heap.o: heap.c
	$(CC) -c $^ $(FLAGS)

clean:
	rm -rf *.o

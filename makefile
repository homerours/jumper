CC=gcc -O3

jumper: jumper.o heap.o
	$(CC) -o $@ $^ 

jumper.o: jumper.c
	$(CC) -c $^

heap.o: heap.c
	$(CC) -c $^

clean:
	rm -rf *.o

CC=gcc
FLAGS=-O3 -lm
# FLAGS+=-Wall -Wextra -Wpedantic \
# 	-Wformat=2 -Wno-unused-parameter -Wshadow \
# 	-Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
# 	-Wredundant-decls -Wnested-externs -Wmissing-include-dirs
# FLAGS+=-fsanitize=address -static-libsan

jumper: jumper.o heap.o record.o matching.o
	$(CC) -o $@ $^ $(FLAGS)

jumper.o: jumper.c
	$(CC) -c $^

heap.o: heap.c
	$(CC) -c $^

record.o: record.c
	$(CC) -c $^

matching.o: matching.c
	$(CC) -c $^

clean:
	rm -rf *.o

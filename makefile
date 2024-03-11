CC=gcc
# FLAGS=-O3 -lm
FLAGS+=-Wall -Wextra -Wpedantic \
	-Wformat=2 -Wno-unused-parameter -Wshadow \
	-Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
	-Wredundant-decls -Wnested-externs -Wmissing-include-dirs
# FLAGS+=-fsanitize=address -static-libsan

jumper: jumper.o heap.o record.o matching.o arguments.o
	$(CC) -o $@ $^ $(FLAGS)

%.o: %.c
	$(CC) -c $^ $(FLAGS)

clean:
	rm -rf *.o

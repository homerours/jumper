CC=gcc
FLAGS=-O3

# dev flags:
# FLAGS=-Wall -Wextra -Wpedantic \
# 	-Wformat=2 -Wno-unused-parameter -Wshadow \
# 	-Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
# 	-Wredundant-decls -Wnested-externs -Wmissing-include-dirs
# FLAGS+=-fsanitize=address -static-libsan

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

install: jumper clean
	@if [ -z $(shell which fzf) ]; then echo "WARNING: FZF not found, fuzzy finding may not work."; fi
	mv $< $(BINDIR)
	@echo "Add 'source jumper.sh' to your bash/zsh rc."

uninstall:
	rm -f $(BINDIR)/jumper
	@echo "Please also remove 'source jumper.sh' from your bash/zsh rc."

jumper: jumper.o heap.o record.o matching.o arguments.o
	@echo 'Compiling jumper...'
	$(CC) -o $@ $^ $(FLAGS) -lm
	@echo 'Success!'

%.o: src/%.c
	$(CC) -c $^ $(FLAGS)

clean:
	rm -f *.o

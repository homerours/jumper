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
	@if [ -d $(BINDIR) ]; then mv $< $(BINDIR); else echo "$(BINDIR) does not exist.\nYou will need to copy the binary ./jumper to a directory in your path ($(PATH))."; fi
	@echo 'Bash: Add '"'"'eval "$$(jumper --shell bash)"'"'"' to your bashrc.'
	@echo "Zsh : Add 'source <(jumper --shell zsh)' to your zshrc."
	@echo "Fish: Add 'jumper --shell fish | source' to your config.fish."

uninstall:
	rm -f $(BINDIR)/jumper

jumper: jumper.o heap.o record.o matching.o arguments.o shell.o
	@echo 'Compiling jumper...'
	$(CC) -o $@ $^ $(FLAGS) -lm
	@echo 'Success!'

%.o: src/%.c
	$(CC) -c $^ $(FLAGS)

clean:
	rm -f *.o

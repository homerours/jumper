# Jumper

Jumper is a directory-jumper program that helps you jumping to / fuzzy-finding the directories that you frequently visit.
It uses [FZF](https://github.com/junegunn/fzf) for fuzzy-finding and is heavily inspired by [z](https://github.com/rupa/z).

## Installation
Use
```bash
git clone https://github.com/homerours/jumper
cd jumper
./install.sh
```
to compile and move the `jumper` binary to `/usr/local/bin`. Then add 
```bash
source jumper.sh
```
to your bash/zsh rc to get access to the jump functions.

## Usage

- Use `j <something>` to jump to the most frequent/recent directories matching `<something>`.
- Use `Ctrl+J` to fuzzy-find the most frequent/recent directories matching `<something>`.
- The general syntax is `jumper -n <number-of-results> <query>`, so `j` is just an alias for `cd $(jumper -n 1 $1)`.

## Principles

The `jumper.sh` adds a `precmd` function that will be executed after each command and will record the `pwd` in the database (a file `~/.j`mimicing what [z](https://github.com/rupa/z) is doing).
Then, the jumper binary is in charge of searching that file.

## Contributing

PR / comments are welcome :)

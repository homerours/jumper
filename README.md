# Jumper

Jumper is a directory-jumper program that helps you jumping to / fuzzy-finding the directories that you frequently visit.
It uses [FZF](https://github.com/junegunn/fzf) for fuzzy-finding and is heavily inspired by [z](https://github.com/rupa/z).

## Installation
Use
```bash
git clone https://github.com/homerours/jumper
cd jumper
sudo ./install.sh
```
to compile and move the `jumper` binary to `/usr/local/bin`. Then add 
```bash
source jumper.sh
```
to your bash/zsh rc to get access to the jump functions.

## Usage
Jumper is made of:
- a `C` program, `jumper` which operates on files whose lines are in the format `<path>|<number-of-visits>|<timestamp-of-last-visit>`. Given such a file
```bash
jumper -f <file> -n N <query>
```
returns the top `N` entries of `<file>` that match `<query>`.

- a shell script (`jumper.sh`) which uses `jumper` for fuzzy-finding and directories jumping:
    - Use `z <something>` to jump to the most frequent/recent directories matching `<something>`.
    - Use `Ctrl+J` to fuzzy-find the most frequent/recent directories matching a query.

## Principles

The `jumper.sh` adds a `precmd` function that will be executed after each command and will record the `pwd` in the database (a file `~/.jumper` mimicing what [z](https://github.com/rupa/z) is doing).

## Contributing

PR / comments are welcome :)

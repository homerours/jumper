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
- a C program, `jumper` which operates on files whose lines are in the format `<path>|<number-of-visits>|<timestamp-of-last-visit>`. Given such a file
```bash
jumper -f <file> -n N <query>
```
returns the top `N` entries of `<file>` that match `<query>`. The command
```bash
jumper -f <file> -a <path>
```
adds the `<path>` to the `<file>`, or updates its data if already present.

- a shell script (`jumper.sh`) which uses `jumper` for fuzzy-finding and directories jumping:
    - Use `z <something>` to jump to the most frequent/recent directories matching `<something>`.
    - Use `Ctrl+J` to fuzzy-find the most frequent/recent directories matching a query.

- Use `__jumper_clean_db` to remove from the database directories that do not exist anymore.

## Vim/Neovim

You can add something like
```
command! -nargs=+ Z :cd `jumper -f ~/.jumper -n 1 <args>`
```
to your `.vimrc` to then jump with `:Z <query>`.

A Telescope extension [telescope-jumper](https://github.com/homerours/telescope-jumper) allows to use jumper within Neovim.

## How it works

The `jumper.sh` adds a `precmd` function that will be executed after each command and will record the `pwd` in the database (a file `~/.jumper` in the same way [z](https://github.com/rupa/z) does).

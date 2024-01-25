# Jumper

Jumper is a program that helps you jumping to / fuzzy-finding the directories and files that you frequently visit.
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
source path/to/jumper.sh
```
to your `bashrc`/`zshrc` to get access to jumper's functions.

## Requirements
- A C compiler for installation. The makefile uses `gcc`.
- Bash (>=4.0) or Zsh.
- [FZF](https://github.com/junegunn/fzf). This is not mandatory, but you will need it for fuzzy-finding.

## Concept
Jumper is a C program, `jumper`, which operates on files whose lines are in the format `<path>|<number-of-visits>|<timestamp-of-last-visit>`. Such file is typically used to record accesses to files/directories. Given such a file, the command
```bash
jumper -f <file> -n N <query>
```
returns the top `N` entries of `<file>` that match `<query>`. The command
```bash
jumper -f <file> -a <path>
```
adds the `<path>` to the `<file>`, or updates its data (increments the visits count and updates the timestamp) if already present.
From these two basic functions, the shell script `jumper.sh` defines various functions/mappings (see next section) allowing to quickly jump around!

## Usage
- Use `z <something>` to jump to the most frequent/recent directories matching `<something>`.
- Use `zf <something>` to open (in `$EDITOR`) the most frequent/recent file matching `<something>`.
- Use `Ctrl+Y` to fuzzy-find the most frequent/recent directories matching a query (FZF required).
- Yse `Ctrl+U` to fuzzy-find the most frequent/recent files matching a query (FZF required).

Database maintainance:
- Use `__jumper_clean_db` to remove from the database directories that do not exist anymore.
- Use `__jumper_clean_files_db` to remove from the database files that do not exist anymore.

In order to populate the directories database, a precommand is added to bash/zsh. To populate the files database, you will have to call `jumper -f ~/.jumper_files -a <full-path-to-file>` everytime you open a file. For Vim/Neovim, this can be done using an `autocmd` (see next section). For other editors, you can for instance use aliases.

## Vim/Neovim

### Telescope extension
A Telescope extension [telescope-jumper](https://github.com/homerours/telescope-jumper) allows to fuzzy-find jumper results within Neovim. 

Below are instructions to use jumper within Vim/Neovim without this plugin.

### Jump to directories
You can add something like
```vim
command! -nargs=+ Z :cd `jumper -f ~/.jumper -n 1 <args>`
```
to your `.vimrc` to then jump with `:Z <query>`.

### Jump to files
In order to log the files you open, add
```vim
autocmd BufReadPre *   silent execute '!jumper -f ~/.jumper_files -a ' .. expand('%:p')
```
or
```lua
vim.api.nvim_create_autocmd({ "BufNewFile", "BufReadPre" }, {
    pattern = { "*" },
    callback = function(ev)
        local filename = vim.api.nvim_buf_get_name(ev.buf)
        if not string.find(filename, ":") then
            local cmd = 'jumper -f ~/.jumper_files -a ' .. filename
            os.execute(cmd)
        end
    end
})
```
to your `.vimrc`/`init.lua`. Then, in order to jump to files, add something like:
```vim
command! -nargs=+ Zf :edit `jumper -f ~/.jumper_files -n 1 <args>`
```

## Combine it with your favorite tools!

You can for instance define a function
```bash
fu() {
    RG_PREFIX="jumper -f ${jumpfile_files} '' | xargs rg -i --column --line-number --color=always "
    INITIAL_QUERY=''
    fzf --ansi --disabled --query "$INITIAL_QUERY" \
    --bind "start:reload:$RG_PREFIX {q}" \
    --bind "change:reload:sleep 0.1; $RG_PREFIX {q} || true" \
    --delimiter : \
    --preview 'bat --color=always {1} --highlight-line {2}' \
    --preview-window 'up,60%,border-bottom,+{2}+3/3,~3' \
    --bind 'enter:become(nvim {1} +{2})'
}
```
which allows to "live-grep" (using here [ripgrep](https://github.com/BurntSushi/ripgrep)) the files of jumper's database.
This is also available in the Telescope extension [telescope-jumper](https://github.com/homerours/telescope-jumper).

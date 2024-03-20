# Jumper

Jumper is a command-line program that helps you jumping to / fuzzy-finding the directories and files that you frequently visit.
It uses [FZF](https://github.com/junegunn/fzf) for fuzzy-finding and is heavily inspired by [z](https://github.com/rupa/z).

It differentiates itself from the plethora of similar tools on the following points:
- It is not restricted to folders. It allows to quickly navigate files, or anything you want (you can easily create and query a new custom database).
- Efficient ranking mechanism which combines the "frecency" of the match (as [z](https://github.com/rupa/z) does) and the accuracy of the match (as [fzf](https://github.com/junegunn/fzf) or [fzy](https://github.com/jhawthorn/fzy) do). More details [here](https://github.com/homerours/jumper/blob/master/doc/algorithm.md).
- It allows fuzzy-finding.
- Written in C, for speed and portability.

## Concept
`jumper` operates on files whose lines are in the format `<path>|<number-of-visits>|<timestamp-of-last-visit>`. Such file is typically used to record accesses to files/directories. Given such a file, the command
```bash
jumper -f <database-file> -n N <query>
```
returns the top `N` entries of the `<database-file>` (this will typically be `~/.jfolders` or `~/.jfiles`) that match `<query>`. Adding the `-c` flag colors the matched substring. The command
```bash
jumper -f <database-file> -a <path>
```
adds the `<path>` to the `<database-file>`, or updates its data (increments the visits count and updates the time stamp) if already present.
From these two main functions, the shell scripts `shell/jumper.{bash,zsh,fish}` define various functions/mappings (see next section) allowing to quickly jump around
- Folders: Folders' visits are recorded in the file `${__JUMPER_FOLDERS}` using a shell pre-command.
- Files: Files open are recorded in the file `${__JUMPER_FILES}` by making Vim run `jumper -f ${__JUMPER_FILES} -a <current-file>` each time a file is open. This can be adapted to other editors.

### Usage
- Use `z <something>` to jump to the most frequent/recent directories matching `<something>`.
- Use `zf <something>` to open (in `$EDITOR`) the most frequent/recent file matching `<something>`.
- Use `Ctrl+Y` to fuzzy-find the most frequent/recent directories matching a query (FZF required).
- Use `Ctrl+U` to fuzzy-find the most frequent/recent files matching a query (FZF required).

All these mappings can be updated in `shell/jumper.{bash,zsh,fish}`.

Database maintenance:
- Use `__jumper_clean_folders_db` to remove from the database directories that do not exist anymore.
- Use `__jumper_clean_files_db` to remove from the database files that do not exist anymore.

### Ranking mechanism

The paths that matches a given query are ranked based on
- the frecency of the path: how often / recently has this path been visited ?
- the accuracy of the query: how well does the query matches the path ?

The ranking of a path at time $t$ is based on the following score
```math
\text{score}(\text{query}, \text{path}) =  \text{frecency}(t, \text{path}) + \beta \times \text{accuracy}(\text{query}, \text{path})
```
where $\beta = 1.0$ by default, but can be updated with the flag `-b <value>`. 
More details about the scoring mechanism are given [here](https://github.com/homerours/jumper/blob/master/doc/algorithm.md).

## Installation process

### Requirements
- A C compiler for installation. The makefile uses `gcc`.
- Bash (>=4.0), Zsh or Fish.
- [FZF](https://github.com/junegunn/fzf). This is not mandatory, but you will need it for fuzzy-finding.

### Installation
Use
```bash
git clone https://github.com/homerours/jumper
cd jumper
make install
```
to compile and move the `jumper` binary to `/usr/local/bin`. Then add 
```bash
source <path-to>/shell/jumper.{bash, zsh, or fish depending on the shell you use}
```
to your `.bashrc`, `.zshrc` or `.config/fish/config.fish` to get access to jumper's functions.

> [!TIP]
> If you were already using [z](https://github.com/rupa/z), you can `cp ~/.z ~/.jfolders` to export your database to Jumper.

## Vim/Neovim

### Telescope extension
A Telescope extension [telescope-jumper](https://github.com/homerours/telescope-jumper) allows to fuzzy-find jumper results within Neovim. 

Below are instructions to use jumper within Vim/Neovim without this plugin.

### Jump to directories and files
You can add something like
```vim
command! -nargs=+ Z :cd `jumper -f ${__JUMPER_FOLDERS} -n 1 '<args>'`
command! -nargs=+ Zf :edit `jumper -f ${__JUMPER_FILES} -n 1 '<args>'`
```
to your `.vimrc` to then change directory with `:Z <query>` or open frequently opened files with `:Zf <query>`.

In order to log the files you open, add
```vim
autocmd BufReadPre,BufNewFile *   silent execute '!jumper -f ${__JUMPER_FILES} -a ' .. expand('%:p')
```
or, if you are using Neovim's lua api,
```lua
vim.api.nvim_create_autocmd({ "BufNewFile", "BufReadPre" }, {
    pattern = { "*" },
    callback = function(ev)
        local filename = vim.api.nvim_buf_get_name(ev.buf)
        -- do not log .git files, and buffers opened by plugins (which often contain some ':')
        if not (string.find(filename, "/.git") or string.find(filename, ":")) then
            local cmd = "jumper -f ${__JUMPER_FILES} -a '" .. filename .. "'" 
            os.execute(cmd)
        end
    end
})
```
to your `.vimrc`/`init.lua`. 

## Combine it with your favorite tools

You can for instance define a function
```bash
fu() {
    RG_PREFIX="jumper -f ${__JUMPER_FILES} '' | xargs rg -i --column --line-number --color=always "
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

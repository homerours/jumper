# Jumper

Jumper is a command-line program that helps you jumping to / fuzzy-finding the directories and files that you frequently visit.
It uses [FZF](https://github.com/junegunn/fzf) for fuzzy-finding and is heavily inspired by [z](https://github.com/rupa/z).

It differentiates itself from the plethora of similar tools on the following points.
- It is not restricted to folders. It allows to quickly navigate files, or anything you want.
- It allows fuzzy-finding.
- His ranking mechanism combines the "frecency" of the match (as [z](https://github.com/rupa/z) does) and the accuracy of the match (as [fzf](https://github.com/junegunn/fzf) or [fzy](https://github.com/jhawthorn/fzy) do). More details below.

## Concept
Jumper is a C program, `jumper`, which operates on files whose lines are in the format `<path>|<number-of-visits>|<timestamp-of-last-visit>`. Such file is typically used to record accesses to files/directories. Given such a file, the command
```bash
jumper -f <file> -n N <query>
```
returns the top `N` entries of `<file>` that match `<query>`. Adding the `-c` flag colors the matched substring. The command
```bash
jumper -f <file> -a <path>
```
adds the `<path>` to the `<file>`, or updates its data (increments the visits count and updates the time stamp) if already present.
From these two basic functions, the shell script `jumper.sh` defines various functions/mappings (see next section) allowing to quickly jump around
- Folders: Folders' visits are recorded in the file `${__JUMPER_FOLDERS}` using a shell pre-command.
- Files: Files open are recorded in the file `${__JUMPER_FILES}` by making Vim run `jumper -f ${__JUMPER_FILES} -a <current-file>` each time a file is open. This can be adapted to other editors.

### Ranking mechanism

The command `jumper -f <file> -n N <query>` outputs the "top N matches" for `<query>`. The matches are ranked using a score, which combines:

- The frecency of the match: this measures the frequency and recency of the visits of the match. Here, we use the formula
```
frecency(match) = log(1 + number-of-visits) * decay(time-since-last-visit)
```
where `decay` is a decaying function.
- How well the match matches the query, quantified by `match_score(query, match)`. Roughly speaking,
```
match_score(query, match) = 10 * number-of-matched-characters - 8 * number-of-gaps + "bonuses"
```

Based on these two values, the final score of the match is
```
score(query, match) = match_score(query, match) + m * frecency(match)
```
where `m = 1.0` by default, but can be updated with the flag `-m <your multiplier>`. 
These additive definition is motivated by the following.
Suppose that one is fuzzy-finding a match, adding one character to `<query>` at a time.
At first, when `<query>` has very few character (typically <=2), the `match_score` will be very small, hence the ranking will be mostly decided by the frecency.
However, as more characters are added, the ranking will favors matches that are more accurate.

## Installation process

### Requirements
- A C compiler for installation. The makefile uses `gcc`.
- Bash (>=4.0) or Zsh.
- [FZF](https://github.com/junegunn/fzf). This is not mandatory, but you will need it for fuzzy-finding.

### Installation
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
to your `.bashrc`/`.zshrc` to get access to jumper's functions.

## Usage
- Use `z <something>` to jump to the most frequent/recent directories matching `<something>`.
- Use `zf <something>` to open (in `$EDITOR`) the most frequent/recent file matching `<something>`.
- Use `Ctrl+Y` to fuzzy-find the most frequent/recent directories matching a query (FZF required).
- Use `Ctrl+U` to fuzzy-find the most frequent/recent files matching a query (FZF required).
All these mappings can be updated in `jumper.sh`.

Database maintenance:
- Use `__jumper_clean_folders_db` to remove from the database directories that do not exist anymore.
- Use `__jumper_clean_files_db` to remove from the database files that do not exist anymore.

In order to populate the directories database, a pre-command is added to Bash/Zsh. To populate the files database, you will have to call `jumper -f ${__JUMPER_FILES} -a <full-path-to-file>` every time you open a file. For Vim/Neovim, this can be done using an `autocmd` (see next section). For other editors, you can for instance use aliases.

## Vim/Neovim

### Telescope extension
A Telescope extension [telescope-jumper](https://github.com/homerours/telescope-jumper) allows to fuzzy-find jumper results within Neovim. 

Below are instructions to use jumper within Vim/Neovim without this plugin.

### Jump to directories
You can add something like
```vim
command! -nargs=+ Z :cd `jumper -f ${__JUMPER_FOLDERS} -n 1 <args>`
```
to your `.vimrc` to then jump with `:Z <query>`.

### Jump to files
In order to log the files you open, add
```vim
autocmd BufReadPre *   silent execute '!jumper -f ${__JUMPER_FILES} -a ' .. expand('%:p')
```
or, if you are using Neovim's lua api,
```lua
vim.api.nvim_create_autocmd({ "BufNewFile", "BufReadPre" }, {
    pattern = { "*" },
    callback = function(ev)
        local filename = vim.api.nvim_buf_get_name(ev.buf)
        -- do not log .git files, and buffers opened by plugins (which often contain some ':')
        if not (string.find(filename, "/.git") or string.find(filename, ":")) then
            local cmd = 'jumper -f ${__JUMPER_FILES} -a ' .. filename
            os.execute(cmd)
        end
    end
})
```
to your `.vimrc`/`init.lua`. Then, in order to jump to files, add something like:
```vim
command! -nargs=+ Zf :edit `jumper -f ${__JUMPER_FILES} -n 1 <args>`
```

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

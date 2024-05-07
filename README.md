# Jumper

Jumper is a command-line program that helps you jumping to the directories and files that you frequently visit.
It relies on [fzf](https://github.com/junegunn/fzf) for fuzzy-finding and is heavily inspired by [z](https://github.com/rupa/z).

It differentiates itself from the plethora of similar tools on the following points:
- Efficient ranking mechanism which combines the "frecency" of the match (as [z](https://github.com/rupa/z) does) and the accuracy of the match (as [fzf](https://github.com/junegunn/fzf) or [fzy](https://github.com/jhawthorn/fzy) do). This allows to find files/folders accurately in very few keystrokes. More details [here](https://github.com/homerours/jumper/blob/master/doc/algorithm.md).
- It is not restricted to folders. It allows to quickly navigate files, or anything you want (you can easily create and query a new custom database).
- It can be run in "interactive mode".
- Written in C, for speed and portability.

[Usage](#usage) - [Installation](#installation) - [Vim-Neovim](#vim)

## Usage

- Use `z <something>` to jump to the most frequent/recent directories matching `<something>`.
- Use `zf <something>` to open (in `$EDITOR`) the most frequent/recent file matching `<something>`.
- Use `Ctrl+Y` to fuzzy-find directories matching a query interactively (`fzf` required).
- Use `Ctrl+U` to fuzzy-find files matching a query interactively (`fzf` required).

All these mappings can be updated, see [Configuration](#configuration) below.

### Ranking mechanism

The paths that match a given query are ranked based on
- *the frecency of the path*: how often / recently has this path been visited ?
- *the accuracy of the query*: how well does the query match the path ?

The ranking of a path at time $t$ is based on the following score
```math
\text{score}(\text{query}, \text{path}) =  \text{frecency}(t, \text{path}) + \beta \times \text{accuracy}(\text{query}, \text{path})
```
where $\beta = 1.0$ by default, but can be updated with the flag `-b <value>`. 
More details about the scoring mechanism are given [here](https://github.com/homerours/jumper/blob/master/doc/algorithm.md).

### Concept

`jumper` operates on files whose lines are in the format `<path>|<number-of-visits>|<timestamp-of-last-visit>`. Such files are typically used to record accesses to files/directories. Given such a file, the command
```bash
jumper -f <database-file> -n N <query>
```
returns the top `N` entries of the `<database-file>` (this will typically be `~/.jfolders` or `~/.jfiles`) that match `<query>`. Adding the `-c` flag colors the matched substring. The command
```bash
jumper -f <database-file> -a <path>
```
adds the `<path>` to the `<database-file>`, or updates its record (i.e. updates the visits count and timestamp) if already present.
From these two main functions, the shell scripts `shell/jumper.{bash,zsh,fish}` define various functions/mappings (see [Usage](#usage) above) allowing to quickly jump around
- **Folders**: Folders' visits are recorded in the file `${__JUMPER_FOLDERS}` using a shell pre-command.
- **Files**: Files open are recorded in the file `${__JUMPER_FILES}` by making Vim run `jumper -f ${__JUMPER_FILES} -a <current-file>` each time a file is opened. This can be adapted to other editors.

### Search syntax

By default jumper uses a simpler version of fzf's "extended search-mode". One can search for multiple tokens separated by spaces, which have to be found in the same order in order to have a match. The full [fzf-syntax](https://github.com/junegunn/fzf?tab=readme-ov-file#search-syntax) is not implemented yet, only the following token are implemented.

| Token     | Match type                 | Description                          |
| --------- | -------------------------- | ------------------------------------ |
| `dotfvi`  | fuzzy-match                | Items that match `dotfvi`            |
| `'wild`   | exact-match (quoted)       | Items that include `wild`            |
| `^music`  | prefix-exact-match         | Items that start with `music`        |
| `.lua$`   | suffix-exact-match         | Items that end with `.lua`           |

The syntax mode can be changed to `fuzzy` (use only fuzzy-matches, the characters `^`, `$` and `'` are interpreted as standard characters) or `exact` (exact matches only), with the `--syntax` flag.

### Case sensitivity

By default, matches are "case-semi-sensitive". This means that a lower case character `a` can match both `a` and `A`, but an upper case character `A` can only match `A`. Matches can be set to be case-sensitive or case-insensitive using the flags `-S` and `-I`.

## Installation

### Requirements
- A C compiler for installation. The makefile uses `gcc`.
- Bash (>=4.0), Zsh or Fish.
- [fzf](https://github.com/junegunn/fzf). This is not mandatory, but you will need it for running queries interactively.

### Installation process

Run
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

In order to keep track of the visited files, the function `jumper -f $__JUMPER_FILES -a <file>` has to be called each time a file `<file>` is opened. 
This can be done automatically in Vim/Neovim, see next section. For other programs, you may want to use aliases (better solutions exist, using for instance "hooks" in emacs) 
```bash
function myeditor() {
   jumper -f $__JUMPER_FILES -a "$1" 
   myeditor $1
}
```

### Configuration

One typically only needs to add `source <path-to>/shell/jumper.{bash, zsh, or fish}` in ones `.bashrc`, `.zshrc` or `.config/fish/config.fish`.
However, the default keybindings, previewers and "database-files" can still be configured if desired. Here is a sample configuration (for bash)
```bash
# Change default folders/files database-files (defaults are ~/.jfolders and ~/.jfiles):
export __JUMPER_FOLDERS='/path/to/custom/database_for_folders'
export __JUMPER_FILES='/path/to/custom/database_for_files'

# Update jumper's options
# Default: '-cH -n 500' (colors the matches, replace $HOME with ~/ and print only the top 500 entries)
__JUMPER_FLAGS='-c -n 1000 --syntax=fuzzy --case-insensitive --beta=0.5'

# Change the default binding (ctrl-p) to toggle preview:
__JUMPER_TOGGLE_PREVIEW='ctrl-o'

# Change default files' previewer (default: bat or cat):
__JUMPER_FZF_FILES_PREVIEW='head -n 30'

# Change default folders' previewer (default: 'ls -1UpC --color=always'):
__JUMPER_FZF_FOLDERS_PREVIEW='ls -lah --color=always'

# IMPORTANT: this has to be after the configuration above:
source /path/to/jumper/shell/jumper.bash

# Change default (ctrl-y and ctrl-u) bindings:
bind -x '"\C-d": jumper-find-dir'
bind -x '"\C-f": jumper-find-file'
```

#### Database maintenance

Use the function `_jumper_clean` to remove from the databases the files and directories that do not exist anymore. To clean the files' or folders' database only, use `__jumper_clean_files_db` or `__jumper_clean_folders_db`.

This cleaning can be done automatically by setting the variable `__JUMPER_CLEAN_FREQ` to some integer value `N`. In such case, the function `_jumper_clean` will be called on average every `N` command run in the terminal.

For more advanced/custom maintenance, the files `~/.jfolders` and `~/.jfiles` can be edited directly.

#### Performance

Querying and updating `jumper`'s database is very fast. On my old 2012 laptop, these operations (over a database with 1000 entries) run in about 4ms:
```bash
$ time for i in {1..100}; do jumper -f ~/.jfolders hello > /dev/null; done
real    0m0.432s
user    0m0.165s
sys     0m0.198s
$ time for i in {1..100}; do ./jumper -f ~/.jfolders -a test; done
real    0m0.383s
user    0m0.118s
sys     0m0.209s
```
For comparison: 
```bash
$ time for i in {1..100}; do wc -l ~/.jfolders > /dev/null; done
real    0m0.357s
user    0m0.117s
sys     0m0.233s
```

## Vim-Neovim

Jumper can be used in Vim and Neovim. Depending on your configuration, you can either use it
- without any plugin, see below. However, you won't be able to do run queries interactively.
- with the [jumper.nvim](https://github.com/homerours/jumper.nvim) plugin (prefered). This uses either [Telescope](https://github.com/nvim-telescope/telescope.nvim) or [fzf-lua](https://github.com/ibhagwan/fzf-lua) as backend for the UI.
- with the [jumper.vim](https://github.com/homerours/jumper.vim) plugin (works for both Vim/Neovim). This uses [fzf "native" plugin](https://github.com/junegunn/fzf/blob/master/README-VIM.md) as UI.

### Without any plugins

We describe below how to use it without plugins. This only allows to use `Z` and `Zf` commands.
First, you have to keep track of the files you open by adding to your `.vimrc`/`init.lua`
```vim
autocmd BufReadPre,BufNewFile *   silent execute '!jumper -f ${__JUMPER_FILES} -a ' .. expand('%:p')
```
or, if you are using Neovim's Lua api,
```lua
vim.api.nvim_create_autocmd({ "BufNewFile", "BufReadPre" }, {
    pattern = { "*" },
    callback = function(ev)
        local filename = vim.api.nvim_buf_get_name(ev.buf)
        -- do not log .git files, and buffers opened by plugins (which often contain some ':')
        if not (string.find(filename, "/.git") or string.find(filename, ":")) then
            vim.fn.system({ "jumper", "-f", os.getenv("__JUMPER_FILES"),  "-a", filename })
        end
    end
})
```
Then in order to quickly jumper to folders and files, add
```vim
command! -nargs=+ Z :cd `jumper -f ${__JUMPER_FOLDERS} -n 1 '<args>'`
command! -nargs=+ Zf :edit `jumper -f ${__JUMPER_FILES} -n 1 '<args>'`
```
to your `.vimrc` to then change directory with `:Z <query>` or open files with `:Zf <query>`.

## Combine it with other tools

You can for instance define a function
```bash
fu() {
    RG_PREFIX="jumper -f ${__JUMPER_FILES} '' | xargs rg -i --column --line-number --color=always "
    fzf --ansi --disabled --query '' \
    --bind "start:reload:$RG_PREFIX {q}" \
    --bind "change:reload:sleep 0.1; $RG_PREFIX {q} || true" \
    --delimiter : \
    --preview 'bat --color=always {1} --highlight-line {2}' \
    --preview-window 'up,60%,border-bottom,+{2}+3/3,~3' \
    --bind 'enter:become(nvim {1} +{2})'
}
```
which allows to "live-grep" (using here [ripgrep](https://github.com/BurntSushi/ripgrep)) the files of jumper's database.

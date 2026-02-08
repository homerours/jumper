# Jumper

Jumper is a command-line program that helps you jumping to the directories and files that you frequently visit, with minimal number of keystrokes.
It relies on [fzf](https://github.com/junegunn/fzf) for UI and is heavily inspired by [z](https://github.com/rupa/z).

https://github.com/homerours/jumper/assets/12702557/5cc45509-9f25-44ff-a69b-e413a7ce57a3

## Why Jumper?

- **🎯 Smart ranking** - Combines frecency (frequency + recency) with match accuracy for precise results in minimal keystrokes
- **🔄 Orderless search** - Match tokens in any order: `z src main` finds both `main/src` and `src/main`
- **📁 Files & directories** - Track and jump to both files and folders, not just directories
- **⚡ Fast** - Written in C for speed (~4ms queries on 1000 entries)
- **🎨 Interactive mode** - Beautiful fzf integration with live preview
- **🔌 Editor integration** - Works with Vim, Neovim, and VSCode

[Read more about the algorithm →](algorithm.md)

## Quick Start

**macOS:**
```bash
brew install homerours/tap/jumper
eval "$(jumper shell bash)"  # or zsh
```

**Linux/Other:**
```bash
# See installation instructions below
```

**That's it!** Start navigating around, then:
```bash
z proj              # Jump to most-used directory matching "proj"
zf main             # Open most-used file matching "main" in $EDITOR
Ctrl+Y              # Interactive directory search (fzf required)
Ctrl+U              # Interactive file search (fzf required)
```

**Quick Links:**
[Installation](#installation) •
[Usage & Examples](#usage) •
[Configuration](#configuration) •
[Editor Integration](#editors) •
[How it Works](#ranking-mechanism)

## Usage

Just move around files and folders as usual - jumper will automatically track them. Then use these commands to jump back quickly:

| Command | Description | Example |
|---------|-------------|---------|
| `z <query>` | Jump to most-used directory | `z proj` → `~/projects/jumper` |
| `zf <query>` | Open most-used file in $EDITOR | `zf main` → opens `src/main.c` |
| `Ctrl+Y` | Interactive directory search (fzf) | Fuzzy-find with live preview |
| `Ctrl+U` | Interactive file search (fzf) | Fuzzy-find with live preview |

### Examples

**Basic navigation:**
```bash
# After visiting ~/projects/jumper/src a few times...
$ z jum src          # Instantly jumps to ~/projects/jumper/src
$ z proj             # Jumps to ~/projects (most frecent match)

# Multiple tokens work in any order (orderless search)
$ z src jum          # Also finds ~/projects/jumper/src
```

**File navigation:**
```bash
# After editing ~/.config/nvim/init.lua...
$ zf init            # Opens ~/.config/nvim/init.lua in $EDITOR
$ zf nvim lua        # Also matches (orderless)
```

**Interactive mode:**
```bash
$ Ctrl+Y             # Opens interactive directory picker
> proj▊              # Type to filter in real-time
  ~/projects/jumper  (154 visits)
  ~/projects/website (87 visits)
  ~/work/project-x   (43 visits)
# Press Enter to jump
```

> **Tip:** All keybindings can be customized. See [Configuration](#configuration) below.

### How It Works

Jumper uses a smart ranking algorithm that combines:
- **Frecency** - How frequently and recently you've visited a path
- **Match accuracy** - How well your query matches the path

This means your most-used paths naturally bubble to the top, but typing more characters gives you precise control.

<details>
<summary><b>📊 Ranking Algorithm Details</b> (click to expand)</summary>

The paths that match a given query are ranked based on:
- *the frecency of the path*: how often / recently has this path been visited?
- *the accuracy of the query*: how well does the query match the path?

The ranking of a path at time $t$ is based on the following score:
```math
\text{score}(\text{query}, \text{path}) =  \text{frecency}(t, \text{path}) + \beta \times \text{accuracy}(\text{query}, \text{path})
```
where $\beta = 1.0$ by default, but can be updated with the flag `-b <value>`.

More details about the scoring mechanism are given in [algorithm.md](https://github.com/homerours/jumper/blob/master/algorithm.md).

</details>

<details>
<summary><b>🗄️ Database Format</b> (click to expand)</summary>

`jumper` records visits to files and directories in files whose lines are in the format `<path>|<number-of-visits>|<timestamp-of-last-visit>`. Given such a database's file, the command
```sh
jumper find -f <database-file> -n N <query>
```
returns the top `N` entries of the `<database-file>` (this will typically be `~/.jfolders` or `~/.jfiles`) that match `<query>`. Adding the `-c` flag colors the matched substring. The command
```sh
jumper update -f <database-file> <path>
```
adds the `<path>` to the `<database-file>`, or updates its record (i.e. updates the visits' count and timestamp) if already present.
From these two main functions, shell scripts (run e.g. `jumper shell bash`) define various functions/mappings (see [Usage](#usage) above) allowing to quickly jump around.
- **Folders**: Folders' visits are recorded in the file `~/.jfolders` using a shell pre-command. This can be updated by setting the `__JUMPER_FOLDERS` environment variable.
- **Files**: Opened files are recorded in the file `~/.jfiles` by making Vim run `jumper update --type=files <current-file>` each time a file is opened. This can be adapted to other editors and the database's file can be updated by setting the `__JUMPER_FILES` environment variable.

</details>

### Search Syntax

By default jumper uses a simpler version of fzf's "extended search-mode". One can search for multiple tokens separated by spaces. The full [fzf-syntax](https://github.com/junegunn/fzf?tab=readme-ov-file#search-syntax) is not implemented yet, only the following token are implemented.

| Token     | Match type                 | Description                          |
| --------- | -------------------------- | ------------------------------------ |
| `dotfvi`  | fuzzy-match                | Items that match `dotfvi`            |
| `'wild`   | exact-match (quoted)       | Items that include `wild`            |
| `^music`  | prefix-exact-match         | Items that start with `music`        |
| `.lua$`   | suffix-exact-match         | Items that end with `.lua`           |

The syntax mode can be changed to `fuzzy` (use only fuzzy-matches, the characters `^`, `$` and `'` are interpreted as standard characters) or `exact` (exact matches only), with the `--syntax` flag.

### Orderless

If the flag `-o` (`--orderless`, name coming from emacs' [orderless](https://github.com/oantolin/orderless) package) is provided, the tokens can be matched in any order. A higher score will be given to matches whose order is closer to the one of the query. More precisely, one adds to the `matching_score` a term proportional to the number of pairs of tokens that are in the right order (see [inversions of a permutations](https://en.wikipedia.org/wiki/Inversion_(discrete_mathematics))).

The `--orderless` flag is turned on by default for the `z` command and interactive searches. This can be changed by editing the `__JUMPER_FLAGS` environment variable.

### Case sensitivity

By default, matches are "case-semi-sensitive". This means that a lower case character `a` can match both `a` and `A`, but an upper case character `A` can only match `A`. Matches can be set to be case-sensitive or case-insensitive using the flags `-S` and `-I`.

## Installation

Jumper runs on Linux and macOS, with either Bash (>=4.0), Zsh or Fish. Installing [fzf](https://github.com/junegunn/fzf) is recommended. This is not mandatory, but needed for running queries interactively.

You'll need to install `jumper` and then set up your shell following the [instructions](#shell) below.
- macOS:
```bash
brew install homerours/tap/jumper
```
- archlinux: an [AUR package](https://aur.archlinux.org/packages/jumper) is available:
```bash
yay -S jumper
```
- other OS: you can either download pre-built packages on the [Releases](https://github.com/homerours/jumper/releases) page, or follow the instructions below to install from source.

### Install from source
A C compiler is needed to install from source. The makefile uses `gcc`.

#### Install script

You can use the install script to clone and compile jumper + set up the shell keybindings automatically:
```sh
PREFIX=$HOME/.local/bin sh -c "$(curl -s https://raw.githubusercontent.com/homerours/jumper/master/install.sh)"
```

#### Manual installation

Alternatively, you can run
```sh
git clone https://github.com/homerours/jumper
cd jumper
make install
```
to compile and move the `jumper` binary to `/usr/local/bin`. You then have to setup your shell as follows.

#### Shell setup<a id='shell'></a>
Add the following to your `.bashrc`, `.zshrc` or `.config/fish/config.fish` to get access to jumper's functions:
* bash
  ```sh
  eval "$(jumper shell bash)"
  ```
* zsh
  ```sh
  source <(jumper shell zsh)
  ```
* fish
  ```fish
  jumper shell fish | source
  ```

> [!TIP]
> If you were already using [z](https://github.com/rupa/z), you can `cp ~/.z ~/.jfolders` to export your database to Jumper.

In order to keep track of the visited files, the function `jumper update --type=files <file>` has to be called each time a file `<file>` is opened. 
This can be done automatically in Vim/Neovim and VSCode, see next section. For other programs, you may want to use aliases (better solutions exist, using for instance "hooks" in emacs) 
```sh
function myeditor() {
   jumper update --type=files "$1" 
   command myeditor $1
}
```

### Configuration

One typically only needs to add the lines above in ones' `.bashrc`, `.zshrc` or `.config/fish/config.fish`.
However, the default keybindings, previewers and "database-files" can still be configured if desired. Here is a sample configuration (for bash)
```sh
# Change default folders/files database-files (defaults are $HOME/.jfolders and $HOME/.jfiles):
export __JUMPER_FOLDERS='/path/to/custom/database_for_folders'
export __JUMPER_FILES='/path/to/custom/database_for_files'

# Update jumper's options
# Default: '-cHo -n 500' (colors the matches, replace $HOME with ~/, orderless, see all options by running 'jumper --help')
__JUMPER_FLAGS='-ceo -n 100 --syntax=fuzzy --case-insensitive --beta=0.5'
# Flags for z/zf functions (default: -o):
__JUMPER_ZFLAGS='-eo'

# FZF options for interactive search
# Default: --height=70% --layout=reverse --keep-right --preview-window=hidden --ansi
__JUMPER_FZF_OPTS='--height=30 --keep-right --preview-window=hidden --ansi'

# Change the default binding (ctrl-p) to toggle preview:
__JUMPER_TOGGLE_PREVIEW='ctrl-o'

# Change default files' previewer (default: bat or cat):
__JUMPER_FZF_FILES_PREVIEW='head -n 30'

# Change default folders' previewer (default: 'ls -1UpC --color=always'):
__JUMPER_FZF_FOLDERS_PREVIEW='ls -lah --color=always'

# IMPORTANT: this has to be after the configuration above:
eval "$(jumper shell bash)"

# Change default (ctrl-y and ctrl-u) bindings:
bind -x '"\C-d": jumper-find-dir'
bind -x '"\C-f": jumper-find-file'
```

#### Database's maintenance

Use `jumper clean` to remove from the databases the files and directories that do not exist anymore. 
To clean the files' or folders' databases only, use `jumper clean --type=files` or `jumper clean --type=directories`.

This cleaning can be done automatically by setting the variable `__JUMPER_CLEAN_FREQ` to some integer value `N`. In such case, the function `jumper clean` will be called on average every `N` command run in the terminal.

For more advanced/custom maintenance, the files `~/.jfolders` and `~/.jfiles` can be edited directly.

#### Performance

Querying and updating `jumper`'s database is very fast and shouldn't cause any latency. On an old 2012 laptop, these operations (over a database with 1000 entries) run in about 4ms:
```sh
$ time for i in {1..100}; do jumper find --type=files hello > /dev/null; done
real    0m0.432s
user    0m0.165s
sys     0m0.198s
$ time for i in {1..100}; do jumper update --type=files test; done
real    0m0.383s
user    0m0.118s
sys     0m0.209s
```
For comparison: 
```sh
$ time for i in {1..100}; do wc -l ~/.jfolders > /dev/null; done
real    0m0.357s
user    0m0.117s
sys     0m0.233s
```

## Editor Integration<a id='editors'></a>

### Vim-Neovim<a id='vim'></a>

Jumper can be used in Vim and Neovim. Depending on your configuration, you can either use it
- without any plugin, see below. However, you won't be able to do run queries interactively.
- with the [jumper.nvim](https://github.com/homerours/jumper.nvim) plugin (prefered). This uses either [Telescope](https://github.com/nvim-telescope/telescope.nvim) or [fzf-lua](https://github.com/ibhagwan/fzf-lua) as backend for the UI.
- with the [jumper.vim](https://github.com/homerours/jumper.vim) plugin (works for both Vim/Neovim). This uses [fzf "native" plugin](https://github.com/junegunn/fzf/blob/master/README-VIM.md) as UI.

#### Without any plugin

We describe below how to use it without plugin. This only allows to use `Z` and `Zf` commands.
First, you have to keep track of the files you open by adding to your `.vimrc`/`init.lua`
```vim
autocmd BufReadPre,BufNewFile *   silent execute '!jumper update --type=files ' .. expand('%:p')
```
or, if you are using Neovim's Lua api,
```lua
vim.api.nvim_create_autocmd({ "BufNewFile", "BufReadPre" }, {
    pattern = { "*" },
    callback = function(ev)
        local filename = vim.api.nvim_buf_get_name(ev.buf)
        -- do not log .git files, and buffers opened by plugins (which often contain some ':')
        if not (string.find(filename, "/.git") or string.find(filename, ":")) then
            vim.fn.system({ "jumper", "update", "--type=files", filename })
        end
    end
})
```
Then in order to quickly jumper to folders and files, add
```vim
command! -nargs=+ Z :cd `jumper find --type=directories -n 1 '<args>'`
command! -nargs=+ Zf :edit `jumper find --type=files -n 1 '<args>'`
```
to your `.vimrc` to then change directory with `:Z <query>` or open files with `:Zf <query>`.

### VSCode

Jumper integrates with VSCode through the [jumper.vscode](https://github.com/homerours/jumper.vscode) extension, which automatically tracks opened files and provides commands to quickly navigate to frequently used files and directories.

**Installation:**
1. Install jumper following the [installation instructions](#installation) above
2. Install the extension from the [VSCode Marketplace](https://marketplace.visualstudio.com/items?itemName=homerours.jumper-vscode) or search for "Jumper" in VSCode's extension panel
3. The extension will automatically track files you open in VSCode

**Features:**
- Automatically updates jumper's database when you open files
- `Jumper: Find File` command to fuzzy-find and open frequently used files
- `Jumper: Find Directory` command to open frequently used directories

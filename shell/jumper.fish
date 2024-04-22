if not set -q __JUMPER_FOLDERS 
    set __JUMPER_FOLDERS ~/.jfolders
end
if not set -q __JUMPER_FILES
    set __JUMPER_FILES ~/.jfiles
end
if not set -q __JUMPER_MAX_RESULTS
    set __JUMPER_MAX_RESULTS 150
end

if not set -q __JUMPER_FZF_FILES_PREVIEW
    if type -q $bat
        set __JUMPER_FZF_FILES_PREVIEW 'bat --color=always'
    else
        set __JUMPER_FZF_FILES_PREVIEW 'cat'
    end
end

if not set -q __JUMPER_FZF_FOLDERS_PREVIEW
    set __JUMPER_FZF_FOLDERS_PREVIEW 'ls -1UpC --color=always'
end
if not set -q __JUMPER_TOGGLE_PREVIEW
    set __JUMPER_TOGGLE_PREVIEW 'ctrl-p'
end

# Create database files if they do not exist
if not test -f $__JUMPER_FOLDERS
    touch "$__JUMPER_FOLDERS"
end
if not test -f $__JUMPER_FILES
    touch "$__JUMPER_FILES"
end

function jumper_update_db --on-event fish_postexec
    if set -q __jumper_current_folder
        if [ $__jumper_current_folder != $PWD ]
            # working directory has changed, this visit has more weight
            jumper -f {$__JUMPER_FOLDERS} -w 1.0 -a "$PWD"
        else
            # working directory has not changed
            jumper -f {$__JUMPER_FOLDERS} -w 0.3 -a "$PWD"
        end
    end
    set -g __jumper_current_folder "$PWD"
end

function z -d "Jump to folder"
    set -l new_path (jumper -f $__JUMPER_FOLDERS -n 1 "$argv")
    if [ -n "$new_path" ]
        cd "$new_path"
    else
        echo 'No match found'
    end
end

function zf -d "Jump to file"
    set -l file (jumper -f $__JUMPER_FILES -n 1 "$argv")
    if [ -n "$file" ]
        eval "$EDITOR '$file'"
    else
        echo 'No match found'
    end
end

function jumper-find-dir -d "Fuzzy-find directories"
    set -l commandline (commandline -t)
    set -l prefix (string match -r -- '^-[^\s=]+=' $commandline)
    set -l __JUMPER "jumper -c -f $__JUMPER_FOLDERS -n $__JUMPER_MAX_RESULTS"
    set result (
    fzf --height=70% --layout=reverse \
        --keep-right \
        --ansi --disabled --query "$argv" \
        --preview "$__JUMPER_FZF_FOLDERS_PREVIEW {}" \
        --preview-window=hidden --bind "$__JUMPER_TOGGLE_PREVIEW:toggle-preview" \
        --bind "start:reload:$__JUMPER {q}" \
        --bind "change:reload:sleep 0.05; $__JUMPER {q} || true"
    )
    commandline -it -- $prefix
    commandline -it -- (string escape $result)
    commandline -f repaint
end

function jumper-find-file -d "Fuzzy-find files"
    set -l commandline (commandline -t)
    set -l prefix (string match -r -- '^-[^\s=]+=' $commandline)
    set -l __JUMPER "jumper -c -f $__JUMPER_FILES -n $__JUMPER_MAX_RESULTS"
    set result (
    fzf --height=70% --layout=reverse \
        --keep-right \
        --ansi --disabled --query "$argv" \
        --preview "$__JUMPER_FZF_FILES_PREVIEW {}" \
        --preview-window=hidden --bind "$__JUMPER_TOGGLE_PREVIEW:toggle-preview" \
        --bind "start:reload:$__JUMPER {q}" \
        --bind "change:reload:sleep 0.05; $__JUMPER {q} || true"
    )
    commandline -it -- $prefix
    commandline -it -- (string escape $result)
    commandline -f repaint
end

bind \cy jumper-find-dir
bind \cu jumper-find-file

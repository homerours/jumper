set __JUMPER_FOLDERS ~/.jfolders
set __JUMPER_FILES ~/.jfiles
set __JUMPER_MAX_RESULTS 150

function jumper_update_db --on-event fish_postexec
    jumper -f {$__JUMPER_FOLDERS} -a "$PWD"
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

function __jumper_fdir -d "Fuzzy-find folders"
    set -l commandline (commandline -t)
    set -l prefix (string match -r -- '^-[^\s=]+=' $commandline)
    set -l __JUMPER "jumper -c -f $__JUMPER_FOLDERS -n $__JUMPER_MAX_RESULTS"
    set result (
    fzf --height=70% --layout=reverse \
        --keep-right \
        --ansi --disabled --query '' \
        --bind "start:reload:$__JUMPER {q}" \
        --bind "change:reload:sleep 0.05; $__JUMPER {q} || true"
    )
    commandline -it -- $prefix
    commandline -it -- (string escape $result)
    commandline -f repaint
end

function __jumper_ffile -d "Fuzzy-find files"
    set -l commandline (commandline -t)
    set -l prefix (string match -r -- '^-[^\s=]+=' $commandline)
    set -l __JUMPER "jumper -c -f $__JUMPER_FILES -n $__JUMPER_MAX_RESULTS"
    set result (
    fzf --height=70% --layout=reverse \
        --keep-right \
        --ansi --disabled --query '' \
        --bind "start:reload:$__JUMPER {q}" \
        --bind "change:reload:sleep 0.05; $__JUMPER {q} || true"
    )
    commandline -it -- $prefix
    commandline -it -- (string escape $result)
    commandline -f repaint
end

bind \cy __jumper_fdir
bind \cu __jumper_ffile

if not set -q __JUMPER_FOLDERS 
    set __JUMPER_FOLDERS ~/.jfolders
end
if not set -q __JUMPER_FILES
    set __JUMPER_FILES ~/.jfiles
end
if not set -q __JUMPER_FLAGS
    set __JUMPER_FLAGS '-cH -n 500'
end
if not set -q __JUMPER_FZF_OPTS
    set __JUMPER_FZF_OPTS --height=70% --layout=reverse --keep-right --preview-window=hidden --ansi
end

if not set -q __JUMPER_FZF_FILES_PREVIEW
    if type -q bat
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

function __jumper_clean_folders_db -d "clean jumper's folders' database"
    set tempfile {$__JUMPER_FOLDERS}_temp
    if test -f $tempfile
        rm $tempfile
    end
    while read -l line;
        set folder (string split -m1 '|' $line)[1]
        if test -d $folder
            echo "$line" >> {$tempfile}
        end
    end < "$__JUMPER_FOLDERS"
    if test -f $tempfile
        mv {$tempfile} {$__JUMPER_FOLDERS}
    end
end

function __jumper_clean_files_db -d "clean jumper's files' database"
    set tempfile {$__JUMPER_FILES}_temp
    if test -f $tempfile
        rm $tempfile
    end
    while read -l line;
        set folder (string split -m1 '|' $line)[1]
        if test -f $folder
            echo "$line" >> {$tempfile}
        end
    end < "$__JUMPER_FILES"
    if test -f $tempfile
        mv {$tempfile} {$__JUMPER_FILES}
    end
end

function __jumper_clean -d "clean jumper's files' and folders' databases"
    __jumper_clean_files_db > /dev/null 2>&1 && __jumper_clean_folders_db > /dev/null 2>&1 &
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
    if [ -n "$__JUMPER_CLEAN_FREQ" ] && [ (math (random) % $__JUMPER_CLEAN_FREQ) -eq 0 ]
        __jumper_clean
    end
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

function __jumper_fdir -d "run fzf on jumper's directories"
    set -l __JUMPER "jumper -c -f $__JUMPER_FOLDERS $__JUMPER_FLAGS"
    fzf $__JUMPER_FZF_OPTS --disabled --query "$argv" \
        --preview "eval x={}; $__JUMPER_FZF_FOLDERS_PREVIEW \$x" \
        --bind "$__JUMPER_TOGGLE_PREVIEW:toggle-preview" \
        --bind "start:reload:$__JUMPER {q}" \
        --bind "change:reload:sleep 0.05; $__JUMPER {q} || true"
end

function __jumper_ffile -d "run fzf on jumper's files"
    set -l __JUMPER "jumper -c -f $__JUMPER_FILES $__JUMPER_FLAGS"
    fzf $__JUMPER_FZF_OPTS --disabled --query "$argv" \
        --preview "eval x={}; $__JUMPER_FZF_FILES_PREVIEW \$x" \
        --bind "$__JUMPER_TOGGLE_PREVIEW:toggle-preview" \
        --bind "start:reload:$__JUMPER {q}" \
        --bind "change:reload:sleep 0.05; $__JUMPER {q} || true"
end

function zi -d "Interactive jump to folder"
	set new_path (__jumper_fdir)
	if [ -n "$new_path" ]
		cd "$new_path"
	end
end

function zfi -d "Interactive jump to file"
    set file (__jumper_ffile)
	if [ -n "$file" ]
		eval "$EDITOR '$file'"
	end
end

function jumper-find-dir -d "Fuzzy-find directories"
    set -l commandline (commandline -t)
    set -l __JUMPER "jumper -c -f $__JUMPER_FOLDERS $__JUMPER_FLAGS"
    set result (__jumper_fdir)
    if [ -n "$result" ]
        commandline -it -- $result
    end
    commandline -f repaint
end

function jumper-find-file -d "Fuzzy-find files"
    set -l commandline (commandline -t)
    set result (__jumper_ffile)
    if [ -n "$result" ]
        commandline -it -- $result
    end
    commandline -f repaint
end

bind \cy jumper-find-dir
bind \cu jumper-find-file

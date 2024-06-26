# Common to bash and zsh
[[ -n $__JUMPER_FOLDERS ]] || export __JUMPER_FOLDERS=~/.jfolders
[[ -n $__JUMPER_FILES ]] || export __JUMPER_FILES=~/.jfiles
[[ -n $__JUMPER_FLAGS ]] || __JUMPER_FLAGS='-cH -n 500'
[[ -n $__JUMPER_FZF_OPTS ]] || __JUMPER_FZF_OPTS=(--height=70% --layout=reverse --keep-right --preview-window=hidden --ansi)

if [[ -z $__JUMPER_FZF_FILES_PREVIEW ]]; then
    if [[ -n $(which bat) ]]; then
        __JUMPER_FZF_FILES_PREVIEW='bat --color=always'
    else
        __JUMPER_FZF_FILES_PREVIEW='cat'
    fi
fi
[[ -n $__JUMPER_FZF_FOLDERS_PREVIEW ]] || __JUMPER_FZF_FOLDERS_PREVIEW='ls -1UpC --color=always'
[[ -n $__JUMPER_TOGGLE_PREVIEW ]] || __JUMPER_TOGGLE_PREVIEW='ctrl-p'

[[ -f ${__JUMPER_FOLDERS} ]] || touch "${__JUMPER_FOLDERS}"
[[ -f ${__JUMPER_FILES} ]] || touch "${__JUMPER_FILES}"

# Jump to most frecent folder
z() {
    args="${@// /\ }"
	new_path=$(jumper -f "${__JUMPER_FOLDERS}" -n 1 "${args}")
	if [[ -z $new_path ]]; then
		echo 'No match found.'
	else
		cd "${new_path}"
	fi
}

# Edit the most frecent file
zf() {
    args="${@// /\ }"
    file=$(jumper -f "${__JUMPER_FILES}" -n 1 "${args}")
	if [[ -z $file ]]; then
		echo 'No match found.'
    else
		eval "${EDITOR} '${file}'"
	fi
}

# Fuzzy-find directories
__jumper_fdir() {
	__JUMPER="jumper -c -f ${__JUMPER_FOLDERS} ${__JUMPER_FLAGS}"
	fzf ${__JUMPER_FZF_OPTS} --disabled --query "$1" \
        --preview "eval x={}; ${__JUMPER_FZF_FOLDERS_PREVIEW} \$x" \
        --bind "${__JUMPER_TOGGLE_PREVIEW}:toggle-preview" \
		--bind "start:reload:${__JUMPER} {q}" \
		--bind "change:reload:sleep 0.05; ${__JUMPER} {q} || true"
}

# Fuzzy-find files
__jumper_ffile() {
	__JUMPER="jumper -c -f ${__JUMPER_FILES} ${__JUMPER_FLAGS}"
	fzf $__JUMPER_FZF_OPTS --disabled --query "$1" \
        --preview "eval x={}; ${__JUMPER_FZF_FILES_PREVIEW} \$x" \
        --bind "${__JUMPER_TOGGLE_PREVIEW}:toggle-preview" \
		--bind "start:reload:${__JUMPER} {q}" \
		--bind "change:reload:sleep 0.05; ${__JUMPER} {q} || true"
}

zi() {
	new_path=$(__jumper_fdir)
	if [[ -n $new_path ]]; then
        # Manually perform tilde expansion
		cd "${new_path/#\~/$HOME}"
	fi
}

zfi() {
    file=$(__jumper_ffile)
	if [[ -n $file ]]; then
        # Manually perform tilde expansion
		$EDITOR "${file/#\~/$HOME}"
	fi
}


# Clean simlinks
__jumper_delete_duplicate_symlinks(){
    tempfile="${__JUMPER_FOLDERS}_temp"
    cp "${__JUMPER_FOLDERS}" "${tempfile}"
    while IFS= read -r line ; do
        entry="${line%%|*}"
        real=$(realpath $entry)
        if [[ $entry != $real ]]; then
            sed -i -e "\:^${real}|:d" "${tempfile}"
        fi
    done < "${__JUMPER_FOLDERS}"
    mv "${tempfile}" "${__JUMPER_FOLDERS}"
}

__jumper_replace_symlinks(){
    tempfile="${__JUMPER_FOLDERS}_temp"
    cp "${__JUMPER_FOLDERS}" "${tempfile}"
    while IFS= read -r line ; do
        entry="${line%%|*}"
        real=$(realpath $entry)
        if [[ $entry != $real ]]; then
            sed -i -e "s:^${real}/:${entry}/:g" "${tempfile}"
        fi
    done < "${__JUMPER_FOLDERS}"
    mv "${tempfile}" "${__JUMPER_FOLDERS}"
}

__jumper_clean_symlinks(){
    __jumper_delete_duplicate_symlinks
    __jumper_replace_symlinks
}

# Remove entries for which the folder does not exist anymore
__jumper_clean_folders_db() {
    tempfile="${__JUMPER_FOLDERS}_temp"
    [[ -f ${tempfile} ]] && rm ${tempfile}
    while IFS=\| read -r entry visits timestamp; do
        if [[ -d $entry ]]; then
            echo "$entry|$visits|$timestamp" >> ${tempfile}
        fi
    done < "${__JUMPER_FOLDERS}"
    [[ -f ${tempfile} ]] && mv "${tempfile}" "${__JUMPER_FOLDERS}"
}

# Remove entries for which the file does not exist anymore
__jumper_clean_files_db() {
    tempfile="${__JUMPER_FILES}_temp"
    [[ -f ${tempfile} ]] && rm ${tempfile}
    while IFS=\| read -r entry visits timestamp; do
        if [[ -f $entry ]]; then
            echo "$entry|$visits|$timestamp" >> ${tempfile}
        fi
    done < "${__JUMPER_FILES}"
    [[ -f ${tempfile} ]] && mv "${tempfile}" "${__JUMPER_FILES}"
}

_jumper_clean() {
    (__jumper_clean_files_db > /dev/null 2>&1 && __jumper_clean_folders_db > /dev/null 2>&1 &)
}

# Database's update
__jumper_update_db() {
    if [[ ! -z $__jumper_current_folder ]]; then
        if [[ $__jumper_current_folder != $PWD ]]; then
            # working directory has changed, this visit has more weight
            jumper -f "${__JUMPER_FOLDERS}" -w 1.0 -a "$PWD"
        else
            # working directory has not changed
            jumper -f "${__JUMPER_FOLDERS}" -w 0.3 -a "$PWD"
        fi
    fi
    __jumper_current_folder=$PWD

    # Remove files and folders that do not exist anymore
    if [[ -n $__JUMPER_CLEAN_FREQ ]] && [[ $(( RANDOM % __JUMPER_CLEAN_FREQ )) == 0 ]]; then
        _jumper_clean
    fi
}

# For Zsh
jumper-find-dir() {
    selected=$(__jumper_fdir)
    LBUFFER="${LBUFFER}${selected}"
    zle reset-prompt
}
jumper-find-file() {
    selected=$(__jumper_ffile)
    LBUFFER="${LBUFFER}${selected}"
    zle reset-prompt
}

# Bindings
zle -N jumper-find-dir
zle -N jumper-find-file
bindkey '^Y' jumper-find-dir
bindkey '^U' jumper-find-file

# Update db
precmd() {
    __jumper_update_db
}

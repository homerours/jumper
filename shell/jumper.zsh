# Common to bash and zsh
export __JUMPER_FOLDERS=~/.jfolders
export __JUMPER_FILES=~/.jfiles
export __JUMPER_MAX_RESULTS=150

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
	__JUMPER="jumper -c -f ${__JUMPER_FOLDERS} -n ${__JUMPER_MAX_RESULTS}"
	fzf --height=70% --layout=reverse \
        --keep-right \
		--ansi --disabled --query '' \
		--bind "start:reload:${__JUMPER} {q}" \
		--bind "change:reload:sleep 0.05; ${__JUMPER} {q} || true"
}

# Fuzzy-find files
__jumper_ffile() {
	__JUMPER="jumper -c -f ${__JUMPER_FILES} -n ${__JUMPER_MAX_RESULTS}"
	fzf --height=70% --layout=reverse \
        --keep-right \
		--ansi --disabled --query '' \
		--bind "start:reload:${__JUMPER} {q}" \
		--bind "change:reload:sleep 0.05; ${__JUMPER} {q} || true"
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
    while IFS= read -r line ; do
        entry="${line%%|*}"
        if [[ -d $entry ]]; then
            echo "$line" >> ${tempfile}
        else
            echo "Removing $entry from database."
        fi
    done < "${__JUMPER_FOLDERS}"
    mv "${tempfile}" "${__JUMPER_FOLDERS}"
}

# Remove entries for which the file does not exist anymore
__jumper_clean_files_db() {
    tempfile="${__JUMPER_FILES}_temp"
    [[ -f ${tempfile} ]] && rm ${tempfile}
    while IFS= read -r line ; do
        entry="${line%%|*}"
        if [[ -f $entry ]]; then
            echo "$line" >> ${tempfile}
        else
            echo "Removing $entry from database."
        fi
    done < "${__JUMPER_FILES}"
    mv "${tempfile}" "${__JUMPER_FILES}"
}


# For Zsh
run-fz() {
    selected=$(__jumper_fdir)
    if [[ -z ${LBUFFER} ]] && [[ ! -z ${selected} ]]; then
        cd $selected
    else
        LBUFFER="${LBUFFER}${selected}"
    fi
    zle reset-prompt
}
run-fz-file() {
    selected=$(__jumper_ffile)
    LBUFFER="${LBUFFER}${selected}"
    zle reset-prompt
}

# Bindings
zle -N run-fz
zle -N run-fz-file
bindkey '^Y' run-fz
bindkey '^U' run-fz-file

# Update db
precmd() {
    __jumper_update_db
}

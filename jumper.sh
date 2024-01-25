export jumpfile=~/.jumper
export jumpfile_files=~/.jumper_files
export __JUMPER_MAX_RESULTS=100

[[ -f ${jumpfile} ]] || touch "${jumpfile}"
[[ -f ${jumpfile_files} ]] || touch "${jumpfile_files}"

# Jump to most frecent file
z() {
	new_path=$(jumper -f "${jumpfile}" -n 1 $1)
	if [[ -z $new_path ]]; then
		echo 'No match found.'
	else
		cd "${new_path}"
	fi
}

# Edit the most frecent file
zf() {
    file=$(jumper -f "${jumpfile_files}" -n 1 $1)
	if [[ -z $file ]]; then
		echo 'No match found.'
    else
		eval "${EDITOR} ${file}"
	fi
}

# Fuzzy-find directories
__jumper_fdir() {
	__JUMPER="jumper -f ${jumpfile} -n ${__JUMPER_MAX_RESULTS}"
	fzf --height=70% --layout=reverse \
		--ansi --disabled --query '' \
		--bind "start:reload:${__JUMPER} {q}" \
		--bind "change:reload:sleep 0.05; ${__JUMPER} {q} || true"
        # --preview 'ls -p1UC --color=always {}' \
}

# Fuzzy-find files
__jumper_ffile() {
	__JUMPER="jumper -f ${jumpfile_files} -n ${__JUMPER_MAX_RESULTS}"
	fzf --height=70% --layout=reverse \
		--ansi --disabled --query '' \
		--bind "start:reload:${__JUMPER} {q}" \
		--bind "change:reload:sleep 0.05; ${__JUMPER} {q} || true"
        # --preview 'bat --color=always {}' \
}


# Add folder to database
__jumper_update_db() {
    jumper -f "${jumpfile}" -a "$PWD"
}

# Clean simlinks
__jumper_delete_duplicate_symlinks(){
    tempfile="${jumpfile}_temp"
    cp "${jumpfile}" "${tempfile}"
    while IFS= read -r line ; do
        entry="${line%%|*}"
        real=$(realpath $entry)
        if [[ $entry != $real ]]; then
            sed -i -e "\:^${real}|:d" "${tempfile}"
        fi
    done < "${jumpfile}"
    mv "${tempfile}" "${jumpfile}"
}

__jumper_replace_symlinks(){
    tempfile="${jumpfile}_temp"
    cp "${jumpfile}" "${tempfile}"
    while IFS= read -r line ; do
        entry="${line%%|*}"
        real=$(realpath $entry)
        if [[ $entry != $real ]]; then
            sed -i -e "s:^${real}/:${entry}/:g" "${tempfile}"
        fi
    done < "${jumpfile}"
    mv "${tempfile}" "${jumpfile}"
}

__jumper_clean_symlinks(){
    __jumper_delete_duplicate_symlinks
    __jumper_replace_symlinks
}

# Remove entries for which the folder does not exist anymore
__jumper_clean_db() {
    tempfile="${jumpfile}_temp"
    [[ -f ${tempfile} ]] && rm ${tempfile}

    while IFS= read -r line ; do
        entry="${line%%|*}"
        if [[ -d $entry ]]; then
            echo "$line" >> ${tempfile}
        else
            echo "Removing $entry from database."
        fi
    done < "${jumpfile}"
    mv "${tempfile}" "${jumpfile}"
}

# Remove entries for which the file does not exist anymore
__jumper_clean_files_db() {
    tempfile="${jumpfile_files}_temp"
    [[ -f ${tempfile} ]] && rm ${tempfile}

    while IFS= read -r line ; do
        entry="${line%%|*}"
        if [[ -f $entry ]]; then
            echo "$line" >> ${tempfile}
        else
            echo "Removing $entry from database."
        fi
    done < "${jumpfile_files}"
    mv "${tempfile}" "${jumpfile_files}"
}

if [[ ! -z ${BASH_VERSION} ]]; then
    # For Bash
    run-fz() {
        selected=$(__jumper_fdir)
        pre="${READLINE_LINE:0:$READLINE_POINT}"
        if [[ -z $pre ]]; then
            cd "$selected"
        else
            READLINE_LINE="${pre}$selected${READLINE_LINE:$READLINE_POINT}"
            READLINE_POINT=$(( READLINE_POINT + ${#selected} ))
        fi
    }
    run-fz-file() {
        selected=$(__jumper_ffile)
        pre="${READLINE_LINE:0:$READLINE_POINT}"
        READLINE_LINE="${pre}$selected${READLINE_LINE:$READLINE_POINT}"
        READLINE_POINT=$(( READLINE_POINT + ${#selected} ))
    }

    # Bindings
	bind -x '"\C-y": run-fz'
    # In some terminals, C-u is bind to kill
    # and can not be remapped.
    stty kill undef
	bind -x '"\C-u": run-fz-file'

    # Update db
	PROMPT_COMMAND="__jumper_update_db;$PROMPT_COMMAND"
else
    # We assume that this is Zsh
    run-fz() {
        selected=$(__jumper_fdir)
        if [[ -z ${LBUFFER} ]]; then
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
fi

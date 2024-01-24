export jumpfile=~/.jumper
export jumpfile_files=~/.jumper_files

[[ -f ${jumpfile} ]] || touch "${jumpfile}"
[[ -f ${jumpfile_files} ]] || touch "${jumpfile_files}"

z() {
	new_path=$(jumper -f "${jumpfile}" -n 1 $1)
	if [[ -z $new_path ]]; then
		echo 'no match found'
	else
		cd "${new_path}"
	fi
}
zf() {
    file=$(jumper -f "${jumpfile_files}" -n 1 $1)
	if [[ -z $file ]]; then
		echo 'no match found'
    else
		eval "${EDITOR} ${file}"
	fi
}

__jumper_fdir() {
	__JUMPER="jumper -f ${jumpfile} -n 20"
	fzf --height=20 --layout=reverse \
        --preview 'ls -p1UC --color=always {}' \
		--ansi --disabled --query '' \
		--bind "start:reload:${__JUMPER} {q}" \
		--bind "change:reload:sleep 0.05; ${__JUMPER} {q} || true"
}

__jumper_ffile() {
	__JUMPER="jumper -f ${jumpfile_files} -n 20"
	fzf --height=20 --layout=reverse \
		--ansi --disabled --query '' \
		--bind "start:reload:${__JUMPER} {q}" \
		--bind "change:reload:sleep 0.05; ${__JUMPER} {q} || true"
}

__jumper_update_db() {
    jumper -f "${jumpfile}" -a "$PWD"
}

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

__jumper_clean_db() {
    # Remove entries for which the folder does not exist anymore
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

__jumper_clean_files_db() {
    # Remove entries for which the folder does not exist anymore
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
    function run-fz() {
        selected=$(__jumper_fdir)
        pre="${READLINE_LINE:0:$READLINE_POINT}"
        if [[ -z $pre ]]; then
            cd "$selected"
        else
            READLINE_LINE="${pre}$selected${READLINE_LINE:$READLINE_POINT}"
            READLINE_POINT=$(( READLINE_POINT + ${#selected} ))
        fi
    }
    function run-fz-file() {
        selected=$(__jumper_ffile)
        pre="${READLINE_LINE:0:$READLINE_POINT}"
        READLINE_LINE="${pre}$selected${READLINE_LINE:$READLINE_POINT}"
        READLINE_POINT=$(( READLINE_POINT + ${#selected} ))
    }
	bind -x '"\C-y": run-fz'
	bind -x '"\C-u": run-fz-file'

    # Update db
	PROMPT_COMMAND="__jumper_update_db;$PROMPT_COMMAND"
else
    # We assume that this is Zsh
	function run-fz {
        selected=$(__jumper_fdir)
        if [[ -z ${LBUFFER} ]]; then
            cd $selected
        else
            LBUFFER="${LBUFFER}${selected}"
        fi
		zle reset-prompt
	}
	function run-fz-file {
        selected=$(__jumper_ffile)
        LBUFFER="${LBUFFER}${selected}"
		zle reset-prompt
	}
	zle -N run-fz
	zle -N run-fz-file
	bindkey '^Y' run-fz
	bindkey '^U' run-fz-file

    # Update db
	precmd() {
		__jumper_update_db
	}
fi

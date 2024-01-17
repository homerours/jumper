export jumpfile=~/.jumper
[[ -f ${jumpfile} ]] || touch "${jumpfile}"

z() {
	new_path=$(jumper -f "${jumpfile}" -n 1 $1)
	if [[ -z $new_path ]]; then
		echo 'no match found'
	else
		cd "${new_path}"
	fi
}

__fz() {
	__JUMPER="jumper -f ${jumpfile} -n 20"
	fzf --height=20 --layout=reverse \
        --preview 'ls -p1UC --color=always {}' \
		--ansi --disabled --query '' \
		--bind "start:reload:${__JUMPER} {q}" \
		--bind "change:reload:sleep 0.05; ${__JUMPER} {q} || true"
}

__update_db() {
	directory=$(pwd)
	now=$(date +%s)
	n=$(perl -i -pe '$k+= s{('"$directory"')\|(\d+)\|(\d+)}{"$1|".($2+1)."|'"$now"'"}ge; END{print "$k"}' "${jumpfile}")
	if [[ $n == '0' ]] || [[ -z $n ]]; then
		echo "${directory}|1|${now}" >> "${jumpfile}"
	fi
}

__clean_db() {
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

if [[ ! -z ${BASH_VERSION} ]]; then
    # For Bash
    function run-fz() {
        selected=$(__fz)
        pre="${READLINE_LINE:0:$READLINE_POINT}"
        if [[ -z $pre ]]; then
            cd "$selected"
            # echo -e "${PS1@P}"
        else
            READLINE_LINE="${pre}$selected${READLINE_LINE:$READLINE_POINT}"
            READLINE_POINT=$(( READLINE_POINT + ${#selected} ))
        fi
    }
	bind -x '"\C-y": run-fz'

    # Update db
	PROMPT_COMMAND="__update_db;$PROMPT_COMMAND"
else
    # We assume that this is Zsh
	function run-fz {
        selected=$(__fz)
        if [[ -z ${LBUFFER} ]]; then
            cd $selected
        else
            LBUFFER="${LBUFFER}${selected}"
        fi
		zle reset-prompt
	}
	zle -N run-fz
	bindkey '^Y' run-fz

    # Update db
	precmd() {
		__update_db
	}
fi

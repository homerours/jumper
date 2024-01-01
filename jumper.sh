export jumpfile=~/.z
j() {
	query=$1
	new_path=$(jumper -f "${jumpfile}" -n 1 $query)
	if [[ -z $new_path ]]; then
		echo 'no match found'
	else
		cd "${new_path}"
	fi
}

fz() {
	RG_PREFIX="jumper -f "${jumpfile}" -n 20"
	INITIAL_QUERY=""
	dest=$(fzf --height=20 --layout=reverse \
		--ansi --disabled --query "$INITIAL_QUERY" \
		--bind "start:reload:$RG_PREFIX {q}" \
		--bind "change:reload:sleep 0.05; $RG_PREFIX {q} || true")
	if [[ ! -z $dest ]]; then
		cd "$dest"
	fi
}

[[ -f ${jumpfile} ]] || touch "${jumpfile}"

__update_db() {
	directory=$(pwd)
	now=$(date +%s)
	n=$(perl -i -pe '$k+= s{('"$directory"')\|(\d+)\|(\d+)}{"$1|".($2+1)."|'"$now"'"}ge; END{print "$k"}' "${jumpfile}")
	if [[ $n == '0' ]]; then
		echo "${directory}|1|${now}" >> "${jumpfile}"
	fi
}

if [[ ! -z ${BASH_VERSION} ]]; then
    # For Bash
	FIRST_PROMPT=1
	function PostCommand() {
		AT_PROMPT=1

		if [ -n "$FIRST_PROMPT" ]; then
			unset FIRST_PROMPT
			return
		fi
		__update_db
	}
	PROMPT_COMMAND="PostCommand"
	bind '"\C-j": "fz\C-m"'
else
    # Assume that this is Zsh
	function run-fz {
		fz
		zle reset-prompt
	}
	zle -N run-fz
	bindkey '^J' run-fz
	precmd() {
		__update_db
	}
fi

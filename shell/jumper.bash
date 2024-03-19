# Common to bash and zsh
source jumper.sh

# For Bash
run-fz() {
    selected=$(__jumper_fdir)
    pre="${READLINE_LINE:0:$READLINE_POINT}"
    if [[ -z $pre ]] && [[ ! -z ${selected} ]]; then
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

# Common to bash and zsh
source jumper.sh

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

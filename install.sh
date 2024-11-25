#!/bin/sh

set -e

CURRENT_DIR=$(pwd -P)
TEMPDIR=$(mktemp -d)

cd "$TEMPDIR"
echo "Cloning jumper's repository..."
git clone https://github.com/homerours/jumper
cd jumper
echo
echo "Building jumper's binary..."
if [ -z "$PREFIX" ]; then
    make install
else
    make PREFIX="$PREFIX" install
fi

cd "$CURRENT_DIR"

echo
echo "Update shell configurations files? (y/n)"
read REPLY
echo

if [ $REPLY = 'y' ]; then

    # Borrowed from https://github.com/junegunn/fzf
    append_line() {
      set -e

      local line file lno
      line="$1"
      file="$2"
      lno=""

      echo "Update $file:"
      echo "  - $line"
      if [ -f "$file" ]; then
        lno=$(\grep -nF "$line" "$file" | sed 's/:.*//' | tr '\n' ' ')
      fi
      if [ -n "$lno" ]; then
        echo "    - Already exists: line #$lno"
      else
        [ -f "$file" ] && echo >> "$file"
        echo "$line" >> "$file"
        echo "    + Added"
      fi
      echo
    }
    
    bash_config=~/.bashrc 
    zsh_config=~/.zshrc 
    fish_config=~/.config/fish/config.fish  
    if [ -f "${bash_config}" ]; then
        append_line 'eval "$(jumper shell bash)"' "${bash_config}" 
    else
        echo "File ${bash_config} does not exists."
        echo "Please add 'eval \"\$(jumper shell bash)\"' to your bash config file if you would like to use jumper's mappings within bash."
        echo
    fi
    if [ -f "${zsh_config}" ]; then
        append_line 'source <(jumper shell zsh)' "${zsh_config}" 
    else
        echo "File ${zsh_config} does not exists."
        echo "Please add 'source <(jumper shell zsh)' to your zsh config file if you would like to use jumper's mappings within zsh."
        echo
    fi
    if [ -f "${fish_config}" ]; then
        append_line 'jumper shell fish | source' "${fish_config}" 
    else
        echo "File ${fish_config} does not exists."
        echo "Please add 'jumper shell fish | source' to your fish config file if you would like to use jumper's mappings within fish."
        echo
    fi
else
    echo 'Shell configuration files have not been updated.'
    echo
fi
echo 'Done!'

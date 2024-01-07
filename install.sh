#!/bin/bash

dest_folder='/usr/local/bin'

echo "Compiling jumper..."

make
make clean

echo "Success. Moving the binary to ${dest_folder}"

mv jumper "${dest_folder}"

echo "Success, installation completed!"

if [[ -z $(which fzf) ]]; then
    echo "Warning: FZF not found, fuzzy finding may not work."
else
    echo "FZF found!"
fi

echo "Add 'source jumper.sh' to your bash/zsh rc."

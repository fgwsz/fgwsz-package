#!/bin/bash

path=$(dirname "$(readlink -f "$0")")

cd $path

mkdir -p ~/bin
ln -s ./fgwsz-package ~/bin/fgwsz-package
echo 'export PATH="$HOME/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc

#! /bin/bash

#
# This array trick is only necessary locally, when
# there might be some old tar ball lying around.
#
# Find the name of the generated tar ball.
bz2_files=($(ls libupnp-*.tar.bz2))
#
# Get the last result of the "ls" command.
# Bash arrays are zero based, Zsh arrays are one based.
# Bash allows negative index.
bz2_name=${bz2_files[-1]}
#
# export results
export bz2_name

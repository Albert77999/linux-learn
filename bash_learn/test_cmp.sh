#!/usr/bin/env bash

# Practice: compare test, [ ], [[ ]], and (( )).

name="${1:-Alice}"
age="${2:-20}"
target="${3:-case.sh}"

# test and [ ] are two forms of the same command.
if test -n "$name"; then
    printf "test: name is not empty\n"
fi

if [ -f "$target" ]; then
    printf "[ ]: %s is a regular file\n" "$target"
else
    printf "[ ]: %s is not a regular file\n" "$target"
fi

# [[ ]] is a Bash keyword with safer string handling and pattern matching.
if [[ "$name" == A* ]]; then
    printf "[[ ]]: name starts with A\n"
fi

# (( )) is designed for arithmetic expressions.
if (( age >= 18 )); then
    printf "(( )): age %s is an adult age\n" "$age"
else
    printf "(( )): age %s is under 18\n" "$age"
fi

if [[ -d "$target" ]]; then
    printf "%s is a directory\n" "$target"
fi
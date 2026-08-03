#!/usr/bin/env bash

# Practice: every command returns an exit status.

target="${1:-case.sh}"

true
status=$?
printf "true returned: %s\n" "$status"

false
status=$?
printf "false returned: %s\n" "$status"

if [[ -f "$target" ]]; then
    printf "The target exists: %s\n" "$target"
else
    printf "The target does not exist: %s\n" "$target"
fi

false && printf "This line is skipped\n"
true || printf "This line is skipped\n"

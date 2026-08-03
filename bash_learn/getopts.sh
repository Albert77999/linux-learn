#!/usr/bin/env bash

# Practice: parse short command-line options with getopts.

usage() {
    printf "Usage: %s [-v] [-n name] [-f file] [-h]\n" "$0"
}

verbose=0
name=""
file="case.sh"

while getopts ":vn:f:h" option; do
    case "$option" in
        v)
            verbose=1
            ;;
        n)
            name="$OPTARG"
            ;;
        f)
            file="$OPTARG"
            ;;
        h)
            usage
            exit 0
            ;;
        :) # 错误分支
            printf "Option -%s requires an argument\n" "$OPTARG" >&2
            usage >&2
            exit 2
            ;;
        \?) # 未知选项
            printf "Unknown option: -%s\n" "$OPTARG" >&2
            usage >&2
            exit 2
            ;;
    esac
done

shift $((OPTIND - 1))

printf "Name: %s\n" "${name:-anonymous}"
printf "File: %s\n" "$file"

if (( verbose )); then
    printf "Verbose mode is enabled\n"
fi

if [[ -f "$file" ]]; then
    printf "File exists\n"
else
    printf "File does not exist\n" >&2
    exit 1
fi
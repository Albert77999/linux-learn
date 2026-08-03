
# 1st 
usage()
{
    printf "Usage: %s -f file [-p pattern] [-n lines] [-h]\n" "$0"
}

FILE=""
PATTERN=""
LINES=5

# fpnh
while getopts ":f:p:n:h" option; do
    case "$option" in
        f)
            FILE="$OPTARG"
            ;;
        p)
            PATTERN="$OPTARG"
            ;;
        n)
            LINES="$OPTARG"
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

if [ $# -ne 0 ]; then
    for arg in "$@"; do
        printf "Unexpected argument: %s\n" "$arg" >&2
    done
    exit 2
fi

# 2nd

if [ -z "$FILE" ]; then
    printf "Option -f is required\n" >&2
    usage >&2
    exit 2
fi

if [ -f "$FILE" ]; then
    printf "File:%s exists\n" "$FILE"
elif [ -e "$FILE" ]; then
    printf "File:%s is not a regular file\n" "$FILE"
    exit 1
else
    printf "File:%s does not exist\n" "$FILE"
    exit 1
fi

# 3rd
printf "Line Count: "
wc -l < "$FILE"
printf "Byte Count: "
wc -c < "$FILE"

# 4th
awk '
    NR ==1 || length($0) > max{
        max = length($0)
        line_number = NR
        }
    END{
        if (NR == 0) {
        print "Empty file"
    } else {
        print "Longest line length:", max
        print "Longest line number:", line_number
    }
    }
' "$FILE"

# 5th
if [ -n "$PATTERN" ]; then
    printf "Pattern: %s\n" "$PATTERN"
    grep -n -- "$PATTERN" "$FILE"
    match_count=$(grep -c -- "$PATTERN" "$FILE")
    printf "Total matching lines: %s\n" "$match_count"
fi

# 6th
if [ "$LINES" -le 0 ]; then
    printf "Invalid number of Lines: %s\n" "$LINES" >&2
    exit 2
fi
printf "First %s lines:\n" "$LINES"
sed -n "1,${LINES}p" "$FILE"

# 7th
# exit 0 : success
# exit 1 : File does not exist or is not a regular file
# exit 2 : Invalid option or argument
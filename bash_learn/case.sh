#!/usr/bin/env bash

# Practice: use case to select one action from several choices.

action="${1:-}" #如果没有传入参数，则使用默认值为空字符串

if [[ -z "$action" ]]; then
    printf "Choose an action (start, stop, status, help,restart,logs): "
    read -r action #让 read 原样读取输入，不把反斜杠 \ 当作转义符
fi

number=0

case "$action" in
    start|run)
        number=1
        ;;
    stop)
        number=2
        ;;
    status)
        number=3
        ;;
    help|-h|--help)
        number=4
        ;;
    restart)
        number=5
        ;;
    logs)
        number=6
        ;;
esac

case "$number" in
    1)
        printf "Starting the service...\n"
        ;;
    2)
        printf "Stopping the service...\n"
        ;;
    3)
        printf "The service is ready.\n"
        ;;
    4)
        printf "Usage: %s {start|stop|status|help}\n" "$0"
        ;;
    5)
        printf "Restarting the service...\n"
        ;;
    6)
        printf "Displaying the service logs...\n"
        ;;
    *)
        printf "Unknown action: %s\n" "$action" >&2
        printf "Try: %s help\n" "$0" >&2
        exit 1
        ;;
esac
#!/bin/bash

set -e
emit_error() {
    file="$(echo "$file_line" | cut -d':' -f1)"
    line="$(echo "$file_line" | cut -d':' -f2)"
    echo -e "\e[1m\e[31mError\e[0m invalid pragma in line \e[35m$file\e[36m:\e[32m$line\e[0m"
    echo -e "\t$code"
    if [[ "$1" != "" ]]; then
        echo -e "\t\e[1mnote:\e[0m $1"
    fi
    exit 1
}

find . -type f | grep -P '(\.c|\.h)$' | while read -r file; do
    grep --with-filename --line-number --only-matching '#pragma.*$' "$file" | while read -r pragma; do
        file_line="$(echo "$pragma" | cut -d':' -f1-2)"
        code="$(echo "$pragma" | cut -d':' -f3-)"
        #shellcheck disable=SC2086
        set -- $code
        case "$1" in
        '#pragma')
            case "$2" in
            omp)
                case "${@:3}" in
                threadprivate*) ;;
                'for nowait') ;;
                section | sections | for | barrier | atomic) ;;
                'critical '?*) ;;
                critical)
                    emit_error "Critical section not named"
                    ;;
                parallel) ;;
                'parallel for') ;;
                'parallel shared') ;;
                *)
                    emit_error
                    ;;
                esac
                ;;
            *)
                emit_error
                ;;
            esac
            ;;
        *)
            emit_error
            ;;
        esac
    done
done

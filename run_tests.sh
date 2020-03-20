#!/bin/bash

set -e
binary="$(grep '^PROG = ' makefile | cut -d'=' -f2 | sed -r 's/ //g')"
test_targets=(serial)
make DFLAGS=-O0 DFLAGS+=-DDEBUG
make release
for mode in debug release; do
    find instances/ -type f |
        sed -r 's/\.[^.]+$//g' |
        sort -V |
        uniq |
        while read -r file; do
            for target in "${test_targets[@]}"; do
                echo '===============================>'
                echo -e "\e[34mRunning test\e[0m"
                echo -e "\tTarget: $target"
                echo -e "\tMode: $mode"
                echo -e "\tFile: $file"
                time output="$(./target/"$mode"/"$target"/"$binary" "$file.in" 2>/dev/null)"
                answer="$(cat "$file.out")"
                if [[ "$output" != "$answer" ]]; then
                    echo -e "\e[31mTest failed\e[0m"
                    echo -e "Expected\n$answer"
                    echo -e "Got\n$output"
                    exit 1
                fi
            done
        done
done

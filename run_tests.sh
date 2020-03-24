#!/bin/bash

set -e
binary="$(grep '^PROG = ' makefile | cut -d'=' -f2 | sed -r 's/ //g')"
make clean
make DFLAGS=-O0 DFLAGS+=-DNDEBUG
make release
for mode in debug release; do
    find instances/ -type f |
        sed -r 's/\.[^.]+$//g' |
        sort -V |
        grep "$1" |
        uniq |
        while read -r file; do
            for target in serial openmp; do
                rm -f /tmp/err
                set +e
                if hash hyperfine &>/dev/null; then
                    output="$(./target/"$mode"/"$target"/"$binary" "$file.in" 2>/tmp/err)"
                    hyperfine "./target/$mode/$target/$binary $file.in"
                else
                    echo '===============================>'
                    echo -e "\e[34mRunning test\e[0m"
                    echo -e "\tTarget: $target"
                    echo -e "\tMode: $mode"
                    echo -e "\tFile: $file"
                    time output="$(./target/"$mode"/"$target"/"$binary" "$file.in" 2>/tmp/err)"
                fi
                answer="$(cat "$file.out")"
                if [[ "$output" != "$answer" ]]; then
                    echo -e "\e[31mTest failed\e[0m"
                    echo -e "Expected\n$answer"
                    echo -e "Got\n$output"
                    echo -e "stderr:"
                    cat /tmp/err
                    exit 1
                fi
            done
        done
done

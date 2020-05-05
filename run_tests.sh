#!/bin/bash

join_by() {
    local IFS="$1"
    shift
    echo "$*"
}

set -e
binary="$(grep '^PROG = ' makefile | cut -d'=' -f2 | sed -r 's/ //g')"
targets=(serial openmp mpi)
modes=(debug)
make clean
make DFLAGS=-O0 DFLAGS+=-DNDEBUG
# make release
if [[ "$1" = bench* ]]; then
    instances=large_instances
else
    instances=instances
fi
for mode in "${modes[@]}"; do
    find "$instances"/ -type f |
        grep '\.in$' |
        sed -r 's/\.[^.]+$//g' |
        sort -V |
        uniq |
        while read -r file; do
            rm -f /tmp/err
            set +e
            if [[ "$1" == bench* ]] && command -v hyperfine &>/dev/null; then
                hyperfine \
                    --runs 2 --export-csv "bench_$(basename "$file")_${mode}.csv" \
                    --export-markdown "bench_$(basename "$file")_${mode}.md" \
                    --export-json "bench_$(basename "$file")_${mode}.json" \
                    --warmup 3 \
                    -L target "$(join_by , "${targets[@]}")" \
                    "./target/$mode/{target}/$binary $file.in"
            else
                for target in "${targets[@]}"; do
                    echo -ne "\e[34mRunning test\e[0m"
                    echo -ne " Target: $target"
                    echo -ne "\tMode: $mode"
                    echo -ne "\tInput: $file"
                    echo -n "....."
                    output="$(command time -o /tmp/time \
                        ./target/"$mode"/"$target"/"$binary" "$file.in" 2>/tmp/err)"
                    answer="$(cat "$file.out")"
                    if [[ "$output" != "$answer" ]]; then
                        echo -e "\e[31m FAILED\e[0m"
                        echo -e "Expected\n$answer"
                        echo -e "Got\n$output"
                        echo -e "stderr:"
                        cat /tmp/err
                    else
                        echo -e "\e[32m ok\e[0m"
                    fi
                    [[ "$1" == bench* ]] && head -1 /tmp/time
                    true # clear $?
                done
            fi
        done
done

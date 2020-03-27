#!/bin/bash

join_by() {
    local IFS="$1"
    shift
    echo "$*"
}

test_output() {
    answer="$(cat "$file.out")"
    if [[ "$output" != "$answer" ]]; then
        echo -e "\e[31mTest failed\e[0m"
        echo -e "Expected\n$answer"
        echo -e "Got\n$output"
        echo -e "stderr:"
        cat /tmp/err
        exit 1
    fi
}

set -e
binary="$(grep '^PROG = ' makefile | cut -d'=' -f2 | sed -r 's/ //g')"
targets=(openmp)
modes=(debug)
make clean
make DFLAGS=-O0 DFLAGS+=-DNDEBUG
make release
clear
if [[ "$1" = bench* ]]; then
    instances=large_instances
else
    instances=instances
fi
for mode in "${modes[@]}"; do
    find "$instances"/ -type f |
        grep -v '\.keep' |
        sed -r 's/\.[^.]+$//g' |
        sort -V |
        uniq |
        while read -r file; do
            rm -f /tmp/err
            set +e
            if hash hyperfine &>/dev/null; then
                if [[ "$1" != bench* ]]; then
                    for target in "${targets[@]}"; do
                        echo "testing output of $mode/$target/$file"
                        output="$(./target/"$mode"/"$target"/"$binary" "$file.in" 2>/tmp/err)"
                        test_output
                    done
                else
                    hyperfine \
                        --runs 2 --export-csv "bench_$(basename "$file")_${mode}.csv" \
                        --export-markdown "bench_$(basename "$file")_${mode}.md" \
                        --export-json "bench_$(basename "$file")_${mode}.json" \
                        --warmup 3 \
                        -L target "$(join_by , "${targets[@]}")" \
                        "./target/$mode/{target}/$binary $file.in"
                fi
            else
                for target in "${targets[@]}"; do
                    echo '===============================>'
                    echo -e "\e[34mRunning test\e[0m"
                    echo -e "\tTarget: $target"
                    echo -e "\tMode: $mode"
                    echo -e "\tFile: $file"
                    time output="$(./target/"$mode"/"$target"/"$binary" "$file.in" 2>/tmp/err)"
                    test_output
                done
            fi
        done
done

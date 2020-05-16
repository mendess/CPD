#!/bin/bash

join_by() {
    local IFS="$1"
    shift
    echo "$*"
}

set -e
binary="$(grep '^PROG = ' makefile | cut -d'=' -f2 | sed -r 's/ //g')"
targets=(serial openmp mpi)
modes=(release debug)
make clean
# make release
if [[ "$1" = bench* ]]; then
    make DFLAGS+=-DNO_ASSERT
    make release
    instances=(instances large_instances)
    # instances=instances
else
    make DFLAGS+=-DNO_ASSERT
    make release
    instances=(instances)
fi
export OMP_NUM_THREADS=4
echo "Testing:"
find "${instances[@]}" -type f | grep '\.in$' | sed -r 's/\.[^.]+$//g' | sort -V | uniq
for mode in "${modes[@]}"; do
    for file in $(find "${instances[@]}" -type f | grep '\.in$' | sed -r 's/\.[^.]+$//g' | sort -V | uniq); do
        rm -f /tmp/err
        set +e
        if false && [[ "$1" == bench* ]] && command -v hyperfine &>/dev/null; then
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
                if [ "$target" = mpi ]; then
                    output="$(command time -o /tmp/time \
                        mpirun ./target/"$mode"/"$target"/"$binary" "$file.in" 2>/tmp/err)"
                else
                    output="$(command time -o /tmp/time \
                        ./target/"$mode"/"$target"/"$binary" "$file.in" 2>/tmp/err)"
                fi
                answer="$(cat "$file.out")"
                if [[ "$output" != "$answer" ]]; then
                    echo -e "\e[31m FAILED\e[0m"
                    [ -n "$SHOW_ERRORS" ] && {
                        echo -e "Expected\n$answer"
                        echo -e "Got\n$output"
                        echo -e "stderr:"
                        cat /tmp/err
                    }
                    true
                else
                    echo -en "\e[32m ok\e[0m"
                    if [[ "$1" == bench* ]]; then
                        echo -n ' took '
                        head -1 /tmp/time
                    else
                        echo
                    fi
                fi
            done
        fi
    done
done

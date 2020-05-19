#!/bin/bash

join_by() {
    local IFS="$1"
    shift
    echo "$*"
}

set -e
binary="$(grep '^PROG = ' makefile | cut -d'=' -f2 | sed -r 's/ //g')"
targets=(mpi)
modes=(debug release)
# make release
if [[ "$1" = bench* ]]; then
    [[ "$2" != nocompile ]] && make
    [[ "$2" != nocompile ]] && make release
    instances=(instances large_instances)
    # instances=instances
else
    [[ "$2" != nocompile ]] && make
    [[ "$2" != nocompile ]] && make release
    instances=(instances)
fi
export OMP_NUM_THREADS=4
NUM_PROCS_MPI="${NUM_PROCS_MPI:-4}"
for mode in "${modes[@]}"; do
    for file in $(find "${instances[@]}" -type f | grep '\.in$' | sed -r 's/\.[^.]+$//g' | sort -V | uniq | grep -v '1e6'); do
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
                [[ "$target" = mpi ]] && printf "%3d" "$NUM_PROCS_MPI"
                echo -ne "\tMode: $mode"
                echo -ne "\tInput: $file"
                echo -n "..... "
                if [ "$target" = mpi ]; then
                    output="$(command time -o /tmp/time \
                        mpirun -n "$NUM_PROCS_MPI" --oversubscribe ./target/"$mode"/"$target"/"$binary" "$file.in" 2>/tmp/err)"
                else
                    output="$(command time -o /tmp/time \
                        ./target/"$mode"/"$target"/"$binary" "$file.in" 2>/tmp/err)"
                fi
                answer="$(cat "$file.out")"
                if [[ "$output" != "$answer" ]]; then
                    echo -e "\e[31mFAILED\e[0m"
                    [ -n "$SHOW_ERRORS" ] && {
                        echo -e "Expected\n$answer"
                        echo -e "Got\n$output"
                        echo -e "stderr:"
                        cat /tmp/err
                        echo "Target $target mode $mode input $file failed" >> failed
                    }
                    true
                else
                    echo -en "\e[32mok\e[0m"
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

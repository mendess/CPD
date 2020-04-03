#!/bin/bash
[ -d benchmaks ] || cd ..

for num in 1 2 4 8 hyper_8; do
    output_file=report/benchmark_${num}.tex
    rm -f $output_file
    {
        cat <<EOF
\\begin{tikzpicture}
    \\begin{axis}[
            ybar,
            enlargelimits=true,
            bar width=20pt,
            width=\\textwidth,
            height=300pt,
            legend style={at={(0.5,-0.15)}, anchor=north,legend columns=-1},
            ylabel={seconds},
            xlabel={instances},
            symbolic x coords={1000,200,400,500,ML100k,ML1M},
            xtick=data,
            nodes near coords,
            nodes near coords align={vertical},
        ]
EOF
        for kind in guided static serial; do
            echo '\addplot coordinates {'
            for file in benchmarks/*h_"$num"_*csv; do
                [[ "$file" = *inst30* ]] && continue
                nfile="$(echo "$file" | sed -r 's/.*inst([^-]+).*\.in.csv/\1/')"
                awk -F',' \
                    -v file="$nfile" \
                    '$9 ~ /'$kind'/ && $9 !~ /old/ {printf "(%s,%.0f)\t%% %s\n", file, $2, $9}' \
                    "$file" |
                    sed -r 's/([0-9]+\.[0-9][0-9])[0-9]*/\1/'
            done
            echo "};"
        done
        cat <<EOF

        \\legend{guided, static, serial}
    \\end{axis}
\\end{tikzpicture}
EOF
    } > "$output_file"
done

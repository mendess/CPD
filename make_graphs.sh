#!/bin/bash

for f in large_instances/*.in; do
    echo "===> $f"
    rm -rf /tmp/plot
    tail -n +4 "$f" | awk '{print $1}' | uniq -c | awk '{print $2" "$1}' >/tmp/plot
    gnuplot <<EOF
    set terminal png truecolor
    set boxwidth 0.8
    set terminal png size 3000,1000
    set style fill solid
    set output "$f.jpg"
    set xrange[0:$(tail -n +4 "$f" | awk '{print $1}' | sort -n | tail -1)]
    plot "/tmp/plot" with boxes
EOF
done

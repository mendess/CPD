set terminal pngcairo font "arial,10" size 400,500
set output 'barchart.png'
set boxwidth 0.75
set style fill solid

plot "data.dat" using 2:xtic(1) with boxes title 'time(s)'
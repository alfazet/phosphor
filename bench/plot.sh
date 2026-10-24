#!/usr/bin/env bash
set -euo pipefail

csv="${1:?usage: $0 results.csv [output.png]}"
out="${2:-timings.png}"

gnuplot <<EOF
set datafile separator ','
set terminal pngcairo size 900,500
set output '$out'

set title 'Render stage timings'
set ylabel 'time (ms)'
set style data histograms
set style histogram rowstacked
set style fill solid 0.8 border -1
set boxwidth 0.7
set key outside right top
set grid ytics

plot '$csv' using 4:xtic(stringcolumn(2).' seed '.stringcolumn(3)) title columnhead, \
     ''     using 5 title columnhead, \
     ''     using 6 title columnhead
EOF

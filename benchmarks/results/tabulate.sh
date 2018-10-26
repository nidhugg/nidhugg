#!/bin/bash

set -euo pipefail

bench=$1
tool=$2
N=$3
shift 2

blame() {
    if grep -Eq '^real ' $1; then
        echo err
    else
        echo t/o
    fi
}

get_traces() {
    pat='^Number of complete executions explored: '
    if grep -Eq "$pat" $1; then
        grep -E "$pat" $1 | cut -d' ' -f6
    elif grep -Eq 'Total executions: ' $1; then
        grep -E 'Total executions: ' $1 | cut -d' ' -f3
    elif grep -Eq '^Trace count: ' $1; then
        # Need to sum with sleepset blocked traces
        line=$(grep -E '^Trace count: ' $1)
        printf "%d+%d\n" $(grep -Po '(?<=: )\d+' <<< "$line") \
               $(grep -Po '(?<=also )\d+' <<< "$line") | bc
    else
        blame $1
    fi
}

get_time() {
    if grep -Eq '^real ' $1; then
        grep -E '^real ' $1 | cut -d' ' -f2
    else
        blame $1
    fi
}

get_mem() {
    if grep -Eq '^res ' $1; then
        grep -E '^res ' $1 | cut -d' ' -f2
    else
        blame $1
    fi
}


echo -e "benchmark\tn\ttool\ttraces\ttime\tmem\n"
for n in $N; do
    f="${tool}_${n}.txt"
    printf "%s\t%d\t%s\t%s\t%s\t%s\n" $bench $n $tool $(get_traces "$f") \
           $(get_time "$f") $(get_mem "$f")
done

#!/bin/bash

set -euo pipefail

verb=$1
shift 1

blame() {
    if grep -Eq '^real ' $1; then
        echo err
    else
        echo '{\timeout}'
    fi
}

get_traces() {
    pat='^Number of complete executions explored: '
    if grep -Eq "$pat" $1; then
        grep -E "$pat" $1 | cut -d' ' -f6
    elif grep -Eq 'Fully\+partially executed traces: ' $1; then
	grep -E 'Fully\+partially executed traces: ' $1 | cut -d' ' -f5
    elif grep -Eq 'Total executions: ' $1; then
        grep -E 'Total executions: ' $1 | cut -d' ' -f3
    elif grep -Eq '^Trace count: ' $1; then
        count=$(grep -E '^Trace count: ' $1 | cut -d' ' -f3)
        if grep -Eq '^Assume-blocked trace count: ' $1; then
            count=$(printf "%d+%d\n" "$count" \
                           $(grep -E '^Assume-blocked trace count: ' $1 | cut -d' ' -f4) | bc)
        fi
        if grep -Eq '^Sleepset-blocked trace count: ' $1; then
            count=$(printf "%d+%d\n" "$count" \
                           $(grep -E '^Sleepset-blocked trace count: ' $1 | cut -d' ' -f4) | bc)
        fi
        printf "%d\n" "$count"
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

case $verb in
    tool)
        bench=$1
        tool=$2
        N=$3
        shift 3
        echo -e "benchmark\tn\ttool\ttraces\ttime\tmem"
        for n in $N; do
            f="${tool}_${n}.txt"
            printf "%s\t%d\t%s\t%s\t%s\t%s\n" $bench $n $tool $(get_traces "$f") \
                   $(get_time "$f") $(get_mem "$f")
        done
        ;;
    wide)
        bench=$1
        tools=$2
        N=$3
        natools=$4
        shift 4
        echo -ne "benchmark\tn"
        for tool in $tools $natools; do
            echo -ne "\t${tool}_traces\t${tool}_time\t${tool}_mem"
        done
        echo ''
        for n in $N; do
            printf "%s\t%d" $bench $n
            for tool in $tools; do
                f="${tool}_${n}.txt"
                traces=$(get_traces "$f")
                if [ x"$traces" = xerr ]; then
                    err='{\error}'
                    printf "\t%s\t%s\t%s" "$err" "$err" "$err"
                else
                    printf "\t%s\t%s\t%s" "$traces" $(get_time "$f") \
                           $(get_mem "$f")
                fi
            done
            na='{\notavail}'
            for tool in $natools; do printf "\t%s\t%s\t%s" "$na" "$na" "$na"; done
            printf "\n"
        done
        ;;
    *)
        echo "Unknown verb $verb" >&2
        exit 1
esac

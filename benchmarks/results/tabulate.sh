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

get_speedup() {
    t=$(get_time $1)
    t1=$(get_time $2)
    if echo "$t" | grep -qe err -e timeout; then
        echo "$t"
    elif echo "$t1" | grep -qe err -e timeout; then
        echo "$t1"
    elif echo "$t" | grep -Eq '^0*(\.0+)?$'; then
        echo nan
    else
        printf "scale=3;%s/%s\n" "$t1" "$t" | bc
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
        T=$4
        shift 4
        echo -e "benchmark\tn\tt\ttool\ttraces\ttime\tspeedup\tmem"
        for n in $N; do
            for t in $T; do
                f="${tool}_${n}_${t}.txt"
                f1="${tool}_${n}_1.txt"
                printf "%s\t%d\t%d\t%s\t%s\t%s\t%s\t%s\n" $bench $n $t $tool \
                       $(get_traces "$f") $(get_time "$f") \
                       $(get_speedup "$f" "$f1") $(get_mem "$f")
            done
        done
        ;;
    wide)
        bench=$1
        tools=$2
        N=$3
        T=$4
        natools=$5
        shift 5
        echo -ne "benchmark\tn\tt"
        for tool in $tools $natools; do
            echo -ne "\t${tool}_traces\t${tool}_time\t${tool}_speedup\t${tool}_mem"
        done
        echo ''
        for n in $N; do
            for t in $T; do
                printf "%s\t%d\t%d" $bench $n $t
                for tool in $tools; do
                    f="${tool}_${n}_${t}.txt"
                    f1="${tool}_${n}_1.txt"
                    traces=$(get_traces "$f")
                    if [ x"$traces" = xerr ]; then
                        err='{\error}'
                        printf "\t%s\t%s\t%s\t%s" "$err" "$err" "$err" "$err"
                    else
                        printf "\t%s\t%s\t%s\t%s" "$traces" $(get_time "$f") \
                               $(get_speedup "$f" "$f1") $(get_mem "$f")
                    fi
                done
                na='{\notavail}'
                for tool in $natools; do printf "\t%s\t%s\t%s\t%s" "$na" "$na" "$na" "$na"; done
                printf "\n"
            done
        done
        ;;
    *)
        echo "Unknown verb $verb" >&2
        exit 1
esac

#!/bin/bash

# Copyright (C) 2018
# A (work-in-progress) test script for running our benchmarks
# Runs all tests, with timing information

DATECMD="date +%Y-%m-%d-%R"
DATE="`${DATECMD}`"

TIMEOUT="600" # 10 mins

# User's timeout 
if [ "$1" == "" ]; then
	echo ">>>>>>>> Timeout is"$TIMEOUT"s <<<<<<<<"
else
    echo ">>>>>>>> Timeout is "$1"s <<<<<<<<"
	TIMEOUT=$1 
fi

RUN="timeout -s 9 "$TIMEOUT

TESTS=" "
# From RCMC
TESTS+=" ./from_RCMC/ainc.c"
TESTS+=" ./from_RCMC/binc.c"
TESTS+=" ./from_RCMC/casrot.c"
TESTS+=" ./from_RCMC/casw.c"
TESTS+=" ./from_RCMC/readers.c"
# From CONCUERROR
TESTS+=" ./from_CONCUERROR/lastzero.c"
# From TRACER
TESTS+=" ./from_TRACER/bakery.c"
TESTS+=" ./from_TRACER/fib_one_variable.c"
TESTS+=" ./from_TRACER/burns.c"
TESTS+=" ./from_TRACER/dijkstra.c"
TESTS+=" ./from_TRACER/peterson.c"
TESTS+=" ./from_TRACER/pgsql.c"
#TESTS+=" ./from_TRACER/filesystem.c"
TESTS+=" ./from_TRACER/n_writes_a_read_two_threads.c"
TESTS+=" ./from_TRACER/n_writers_a_reader.c"
#TESTS+=" ./from_TRACER/exponential_bug.c"
TESTS+=" ./from_TRACER/control_flow.c"
TESTS+=" ./from_TRACER/myexample.c"
TESTS+=" ./from_TRACER/redundant_co.c"
TESTS+=" ./from_TRACER/CoRR2.c"
TESTS+=" ./from_TRACER/alpha1.c"
TESTS+=" ./from_TRACER/alpha2.c"
TESTS+=" ./from_TRACER/CO-2+2W.c"
TESTS+=" ./from_TRACER/CO-MP.c"
TESTS+=" ./from_TRACER/CO-R.c"
TESTS+=" ./from_TRACER/CO-S.c"
TESTS+=" ./from_TRACER/co1.c"
TESTS+=" ./from_TRACER/co10.c"
TESTS+=" ./from_TRACER/co4.c"
# SV-COMP
TESTS+=" ./SV-COMP/sigma.c"
TESTS+=" ./SV-COMP/dekker.c"
TESTS+=" ./SV-COMP/szymanski.c"
TESTS+=" ./SV-COMP/lamport.c"
TESTS+=" ./SV-COMP/fibonacci.c"
#TESTS+=" ./SV-COMP/stack_true.c"
#TESTS+=" ./SV-COMP/queue_ok.c"
#TESTS+=" ./SV-COMP/pthread_demo.c"
#TESTS+=" ./SV-COMP/gcd.c"
#TESTS+=" ./SV-COMP/indexer.c"
# from_SCTBench
TESTS+=" ./from_SCTBench/circular_buffer.c"


TOOL_NIDHUGG="nidhuggc"
MODEL_ARGS_NIDHUGG=" --sc "
#
MODEL_ARGS_NIDHUGG_OBS=" --sc --observers --optimal "
#
TOOL_SWSC="swscc"
MODEL_ARGS_SWSC=" --wsc "

COUNT=0

function run_test {
	TOOL=$1
	shift
	t=$1
	shift

	ARGS=$@

	echo "-----------------------------------------------"
	echo 
	echo "< Running test ${COUNT} (${t}) with tool ${TOOL} and argument ${ARGS} >"
	(time ${RUN} ${TOOL} ${ARGS} ${t}  2>&1) 2>&1
	echo
	echo "Test done; sleeping for a few seconds"
	echo

	let COUNT++
}


function run_all_tests {
	echo ${DATE}
	
	COUNT=0
	for t in ${TESTS}
	do
		run_test ${TOOL_NIDHUGG} ${t} ${MODEL_ARGS_NIDHUGG} 
	done

	COUNT=0
	for t in ${TESTS}
	do
		run_test ${TOOL_NIDHUGG} ${t} ${MODEL_ARGS_NIDHUGG_OBS} 
	done
	
	COUNT=0
	for t in ${TESTS}
	do
		run_test ${TOOL_SWSC} ${t} ${MODEL_ARGS_SWSC}  
	done


}

run_all_tests |& tee ./log-files/runall.log

python3 ./gen_table.py

cat ./result-table.txt

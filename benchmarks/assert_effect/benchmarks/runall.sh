#!/bin/bash

# Copyright (C) 2018
# A (work-in-progress) test script for running our benchmarks
# Runs all tests, with timing information

DATECMD="date +%Y-%m-%d-%R"
DATE="`${DATECMD}`"

TIMEOUT="600" # 10 mins

# User's timeout 
if [ "$1" == "" ]; then
	echo ">>>>>>>> Timeout is "$TIMEOUT"s <<<<<<<<"
else
    echo ">>>>>>>> Timeout is "$1"s <<<<<<<<"
	TIMEOUT=$1 
fi

RUN="timeout -s 9 "$TIMEOUT

TESTS=" "
# SV-COMP
TESTS+=" ./SV-COMP/dekker.c"
TESTS+=" ./SV-COMP/fibonacci.c"
TESTS+=" ./SV-COMP/indexer.c"
TESTS+=" ./SV-COMP/lamport.c"
TESTS+=" ./SV-COMP/szymanski.c"
TESTS+=" ./SV-COMP/stack_true.c"
TESTS+=" ./SV-COMP/queue_ok.c"
# From TRACER
TESTS+=" ./from_TRACER/bakery.c"
TESTS+=" ./from_TRACER/fib_one_variable.c"
TESTS+=" ./from_TRACER/burns.c"
TESTS+=" ./from_TRACER/dijkstra.c"
TESTS+=" ./from_TRACER/peterson.c"
TESTS+=" ./from_TRACER/pgsql.c"
#TESTS+=" ./from_TRACER/exponential_bug.c"
TESTS+=" ./from_TRACER/CO-2+2W.c"
# From DCDPOR
TESTS+=" ./from_DCDPOR/parker.c"
# Other benchmarks do not contain assertions

# Nidhugg with optimal option
TOOL_NIDHUGG="nidhuggc"
MODEL_ARGS_NIDHUGG=" --sc --optimal "
# Nidhugg with observer
MODEL_ARGS_NIDHUGG_OBS=" --sc --observers --optimal "
# SWSC
TOOL_SWSC="/home/magnus/swsc/build_art/src/swscc"
MODEL_ARGS_SWSC=" --wsc "
# RCMC with rc11 option
TOOL_RCMC_RC11="rcmc"
MODEL_ARGS_RCMC_RC11=" --rc11 -- "
# RCMC with wrc11 option
TOOL_RCMC_WRC11="rcmc"
MODEL_ARGS_RCMC_WRC11=" --wrc11 -- "

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

  COUNT=0
  for t in ${TESTS}
  do
  run_test ${TOOL_RCMC_RC11} ${t} ${MODEL_ARGS_RCMC_RC11}
  done

  COUNT=0
  for t in ${TESTS}
  do
  run_test ${TOOL_RCMC_WRC11} ${t} ${MODEL_ARGS_RCMC_WRC11}
  done

}

run_all_tests |& tee ./log-files/runall.log

python3 ./gen_table.py

cat ./result-table.txt

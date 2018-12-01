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
# from_DCDPOR
TESTS+=" ./from_DCDPOR/opt_lock.c"
# from_OBSERVERS
TESTS+=" ./from_OBSERVERS/floating_read.c"
# from_RCMC
TESTS+=" ./from_RCMC/casw.c"
# from_TRACER
TESTS+=" ./from_TRACER/CO-MP.c"
TESTS+=" ./from_TRACER/CO-R.c"
TESTS+=" ./from_TRACER/CO-S.c"
TESTS+=" ./from_TRACER/CoRR2.c"
TESTS+=" ./from_TRACER/alpha1.c"
TESTS+=" ./from_TRACER/alpha2.c"
TESTS+=" ./from_TRACER/co1.c"
TESTS+=" ./from_TRACER/co4.c"
TESTS+=" ./from_TRACER/control_flow.c"
TESTS+=" ./from_TRACER/exponential_bug.c"
TESTS+=" ./from_TRACER/myexample.c"
TESTS+=" ./from_TRACER/n_writers_a_reader.c"
TESTS+=" ./from_TRACER/n_writes_a_read_two_threads.c"
TESTS+=" ./from_TRACER/redundant_co.c"
# synthetic
TESTS+=" ./synthetic/length_parametric.c"
TESTS+=" ./synthetic/race_parametric.c"
TESTS+=" ./synthetic/race_parametric_assert.c"
TESTS+=" ./synthetic/sat-example.c"
TESTS+=" ./synthetic/thread_parametric.c"




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

#!/usr/bin/python3

# Copyright (C) 2018
# Present experimental results in table

import sys
import tempfile
import os

from mylibrary import get_nice_time
from mylibrary import get_nice_etime
from mylibrary import get_nice_tracecount
from mylibrary import get_result
 
curDir = os.getcwd()
LOGFILE = curDir + '/log-files/runall.log'
TABLEFILE = curDir + '/result-table.txt'

tests = []
# SV-COMP
#tests.append({'tstname' : 'pthread_demo'})
#tests.append({'tstname' : 'gcd'})
tests.append({'tstname' : 'fibonacci'})
#tests.append({'tstname' : 'indexer'})
tests.append({'tstname' : 'sigma'})
tests.append({'tstname' : 'dekker'})
tests.append({'tstname' : 'szymanski'})
tests.append({'tstname' : 'lamport'})
#tests.append({'tstname' : 'stack_true'})
#tests.append({'tstname' : 'queue_ok'})
# From RCMC
tests.append({'tstname' : 'ainc'})
tests.append({'tstname' : 'binc'})
tests.append({'tstname' : 'casrot'})
tests.append({'tstname' : 'casw'})
tests.append({'tstname' : 'readers'})
# From CONCUERROR
tests.append({'tstname' : 'lastzero'})
# From TRACER
tests.append({'tstname' : 'bakery'})
tests.append({'tstname' : 'fib_one_variable'})
tests.append({'tstname' : 'burns'})
tests.append({'tstname' : 'dijkstra'})
tests.append({'tstname' : 'peterson'})
tests.append({'tstname' : 'pgsql'})
#tests.append({'tstname' : 'filesystem'})
tests.append({'tstname' : 'n_writes_a_read_two_threads'})
tests.append({'tstname' : 'n_writers_a_reader'})
#tests.append({'tstname' : 'exponential_bug'})
tests.append({'tstname' : 'control_flow'})
tests.append({'tstname' : 'myexample'})
tests.append({'tstname' : 'redundant_co'})
tests.append({'tstname' : 'CoRR2'})
tests.append({'tstname' : 'alpha1'})
tests.append({'tstname' : 'alpha2'})
tests.append({'tstname' : 'CO-2+2W'})
tests.append({'tstname' : 'CO-MP'})
tests.append({'tstname' : 'CO-R'})
tests.append({'tstname' : 'CO-S'})
tests.append({'tstname' : 'co1'})
tests.append({'tstname' : 'co10'})
tests.append({'tstname' : 'co4'})


def write_a_line(outputfile, name, execs1, execs2, execs3, time1, time2, time3):
	outputfile.write(' {0:27} '.format(name))
	outputfile.write(' {0:>7} '.format(execs1))
	outputfile.write(' {0:>7} '.format(execs2))
	outputfile.write(' {0:>7} '.format(execs3))
	outputfile.write(' ')
	outputfile.write(' {0:>9} '.format(time1))
	outputfile.write(' {0:>9} '.format(time2))
	outputfile.write(' {0:>9}\n'.format(time3))


def main():

	for tst in tests:
		get_result(tst, LOGFILE, "nidhugg")
		get_result(tst, LOGFILE, "observer")
		get_result(tst, LOGFILE, "swsc")

	outputfile=open(TABLEFILE,'w')
	
	cols = 90
	outputfile.write('=' * cols + '\n')
	outputfile.write("                              --- Number of Traces ----   ------- Execution Time -------- \n")
	outputfile.write(" Program name                 Nidhugg  Observers   SWSC    Nidhugg   Observers      SWSC\n")
	outputfile.write('=' * cols + '\n')
	for tst in tests:
		#print('name:'+tst['tstname']+'\n')
		#print('cds exes'+tst['CDSChecker execs']+'\n')
		write_a_line(outputfile, tst['tstname'], tst['nidhugg execs'], tst['observer execs'], tst['swsc execs'], tst['nidhugg time'], tst['observer time'], tst['swsc time'])

	outputfile.write('=' * cols + '\n')
	outputfile.close()


main()


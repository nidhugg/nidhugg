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
tests.append({'tstname' : 'fibonacci'})
tests.append({'tstname' : 'indexer'})
tests.append({'tstname' : 'dekker'})
tests.append({'tstname' : 'szymanski'})
tests.append({'tstname' : 'lamport'})
tests.append({'tstname' : 'stack_true'})
tests.append({'tstname' : 'queue_ok'})
### From TRACER
tests.append({'tstname' : 'bakery'})
tests.append({'tstname' : 'fib_one_variable'})
tests.append({'tstname' : 'burns'})
tests.append({'tstname' : 'dijkstra'})
tests.append({'tstname' : 'peterson'})
tests.append({'tstname' : 'pgsql'})
#tests.append({'tstname' : 'exponential_bug'})
tests.append({'tstname' : 'CO-2+2W'})
# From DCDPOR
tests.append({'tstname' : 'parker'})
# Other benchmarks do not contain assertions

def write_a_line(outputfile, name, execs1, execs2, execs3, execs4, execs5, time1, time2, time3, time4, time5):
	outputfile.write(' {0:27} '.format(name))
	outputfile.write(' {0:>9} '.format(execs1))
	outputfile.write(' {0:>9} '.format(execs2))
	outputfile.write(' {0:>9} '.format(execs3))
	outputfile.write(' {0:>9} '.format(execs4))
	outputfile.write(' {0:>9} '.format(execs5))
	outputfile.write('{0:>7} '.format(" "))
	outputfile.write(' {0:>12} '.format(time1))
	outputfile.write(' {0:>12} '.format(time2))
	outputfile.write(' {0:>12} '.format(time3))
	outputfile.write(' {0:>12} '.format(time4))
	outputfile.write(' {0:>12}\n'.format(time5))


def main():

	for tst in tests:
		get_result(tst, LOGFILE, "nidhugg")
		get_result(tst, LOGFILE, "observer")
		get_result(tst, LOGFILE, "swsc")
		get_result(tst, LOGFILE, "--rc11")
		get_result(tst, LOGFILE, "--wrc11")

	outputfile=open(TABLEFILE,'w')
	
	cols = 160
	outputfile.write('=' * cols + '\n')
	outputfile.write("                              ------------------ Number of Traces -------------------        -------------------------- Execution Time -------------------------- \n")
	outputfile.write(' {0:27} '.format("Program name"))
	outputfile.write(' {0:>9} '.format("Optimal"))
	outputfile.write(' {0:>9} '.format("Observers"))
	outputfile.write(' {0:>9} '.format("SWSC"))
	outputfile.write(' {0:>9} '.format("RC11"))
	outputfile.write(' {0:>9} '.format("WRC11"))
	outputfile.write('{0:>7} '.format(" "))
	outputfile.write(' {0:>12} '.format("Optimal"))
	outputfile.write(' {0:>12} '.format("Observers"))
	outputfile.write(' {0:>12} '.format("SWSC"))
	outputfile.write(' {0:>12} '.format("RC11"))
	outputfile.write(' {0:>12}\n'.format("WRC11"))
	#outputfile.write(" Program name                 Nidhugg-Opt  Observers   SWSC   RCMC(rc11)   RCMC(wrc11)       Nidhugg-Otp   Observers      SWSC      RCMC(rc11)    RCMC(wrc11)\n")
	outputfile.write('=' * cols + '\n')
	for tst in tests:
		#print('name:'+tst['tstname']+'\n')
		#print('cds exes'+tst['CDSChecker execs']+'\n')
		write_a_line(outputfile, tst['tstname'], tst['nidhugg execs'], tst['observer execs'], tst['swsc execs'],tst['--rc11 execs'], tst['--wrc11 execs'],
                             tst['nidhugg time'], tst['observer time'], tst['swsc time'], tst['--rc11 time'], tst['--wrc11 time'])

	outputfile.write('=' * cols + '\n')
	outputfile.close()


main()


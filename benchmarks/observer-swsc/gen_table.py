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
# From DCDPOR
tests.append({'tstname' : 'opt_lock'})
# From Observers
tests.append({'tstname' : 'floating_read'})
# From RCMC
tests.append({'tstname' : 'casw'})
### From TRACER
tests.append({'tstname' : 'CO-MP'})
tests.append({'tstname' : 'CO-R'})
tests.append({'tstname' : 'CO-S'})
tests.append({'tstname' : 'CoRR2'})
tests.append({'tstname' : 'alpha1'})
tests.append({'tstname' : 'alpha2'})
tests.append({'tstname' : 'co1'})
tests.append({'tstname' : 'co4'})
tests.append({'tstname' : 'control_flow'})
tests.append({'tstname' : 'exponential_bug'})
tests.append({'tstname' : 'myexample'})
tests.append({'tstname' : 'n_writers_a_reader'})
tests.append({'tstname' : 'n_writes_a_read_two_threads'})
tests.append({'tstname' : 'redundant_co'})
## From synthetic
tests.append({'tstname' : 'length_parametric'})
tests.append({'tstname' : 'race_parametric'})
tests.append({'tstname' : 'race_parametric_assert'})
tests.append({'tstname' : 'sat-example'})
tests.append({'tstname' : 'thread_parametric'})

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
	outputfile.write('=' * cols + '\n')
	for tst in tests:
		write_a_line(outputfile, tst['tstname'], tst['nidhugg execs'], tst['observer execs'], tst['swsc execs'],tst['--rc11 execs'], tst['--wrc11 execs'],
                             tst['nidhugg time'], tst['observer time'], tst['swsc time'], tst['--rc11 time'], tst['--wrc11 time'])

	outputfile.write('=' * cols + '\n')
	outputfile.close()


main()


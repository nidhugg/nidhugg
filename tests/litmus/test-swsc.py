#!/usr/bin/python3

# Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
# Based on source code of Carl Leonardsson
# Run swsc on all C litmus tests.

import datetime
import subprocess
import sys
import tempfile
import os
import time

curDir = os.getcwd()
LITMUSDIR = curDir + '/C-litmus'
OUTPUTTFILE = curDir + '/swsc.results.txt'
LISTFILE = curDir + '/nidhugg.results.txt'
# FIX ME
SWSCBIN = 'swscc'




def get_expected(fname):
    f = open(fname,'r')
    l = []

    for ln in f:
        ln = ln.strip()
        if len(ln) and not(ln[0] == '#'):
            segments = ln.split()
            assert(segments[1] == 'Allow' or segments[1] == 'Forbid')
            l.append({'tstname' : segments[0],
                      'expect allow' : (segments[1] == 'Allow'),
                      'expected trace count' : int(segments[3])})
    f.close()

    return l





def res_to_string(tst):
    s = tst['tstname']
    
    if not('swsc allow' in tst):
        s = s + ' FAILURE: '
        if 'failure' in tst:
            s = s + tst['failure']
        else:
            s = s + '(unknown)'
        return s

    if tst['swsc allow'] != tst['expect allow']:
        s = s + ' FAILURE: unexpected result ' + ('Allow' if tst['nidhugg allow'] else 'Forbid')
        return s
    else:
    	s = s + ' ' + ('Allow' if tst['swsc allow'] else 'Forbid')

    if tst['expected trace count'] != tst['swsc trace count']:
    	s = s + ' FAILURE: not same trace as ' + str(tst['expected trace count'])

    s = s + ' OK '
    
    return s





    
def runallswsc():
    logfile = open(OUTPUTTFILE, 'w')
    logfile.write('# Results of running swsc to all C litmus tests.\n')
    version = subprocess.check_output([SWSCBIN, '--version'], stderr = subprocess.STDOUT).decode()
    logfile.write('# ' + version)
    logfile.write('# swsc --wsc TEST.c \n')
    logfile.write('# The tests where executed using test-swsc.py.\n')
    logfile.write('# Date: ' + datetime.datetime.now().strftime('%y%m%d-%H:%M')+'\n')
    logfile.write('\n')
    
    totaltracecount = 0
    tracecount = 0
    tests = get_expected(LISTFILE)
    n = 0
    t0 = time.time()

    for tst in tests:
        n = n + 1
        try:
            out = subprocess.check_output([SWSCBIN, '--wsc',
                                           LITMUSDIR + '/' + tst['tstname'] + '.c'],
                                          stderr = subprocess.STDOUT).decode()
            parts_0 = out.split("\n")
            error_info = parts_0[-6]
            trace_info = parts_0[-7]
            tracecount = int(trace_info.split(":")[1].split()[0])
            totaltracecount += tracecount
            if out.find('No errors were detected') >= 0:
                tst['swsc allow'] = False
            else:
                tst['swsc allow'] = True
        except subprocess.CalledProcessError:
            tst['failure'] = 'swsc'
            tracecount = -1
            continue
        finally:
            tst['swsc trace count'] = tracecount
            print('{0:4}: '.format(n), end = '')
            print(res_to_string(tst) + ' : ' + str(tracecount))
            logfile.write(res_to_string(tst) + ' : ' + str(tracecount) + '\n')

    runtime = time.time() - t0
    logfile.write('# Total number of traces: ' + str(totaltracecount) + '\n')
    logfile.write('# Total running time: {0:.2f} s\n'.format(runtime))
    logfile.close()






def runoneswsc(filename):
    tracecount = 0
    found = False

    for tst in tests:
        if tst['tstname'] != filename:
            continue
        else:
            found = True
            try:
                out = subprocess.check_output([SWSCBIN, '--wsc',
                                               LITMUSDIR + '/' + tst['tstname'] + '.c'],
                                              stderr = subprocess.STDOUT).decode()
                parts_0 = out.split("\n")
                error_info = parts_0[-6]
                trace_info = parts_0[-7]
                tracecount = int(trace_info.split(":")[1].split()[0])
                if out.find('No errors were detected') >= 0:
                    tst['swsc allow'] = False
                else:
                    tst['swsc allow'] = True
            except subprocess.CalledProcessError:
                tst['failure'] = 'swsc'
                tracecount = -1
            finally:
                tst['swsc trace count'] = tracecount
                print(res_to_string(tst) + ' : ' + str(tracecount))
                break

    if found == False:
        print('Test case was not found!')
        




def help_print():
    print('Possible commands:')
    print('\t1. python3 ./test-swsc.py all')
    print('\t2. python3 ./test-swsc.py one testname(without_dot_c)')


    
    
####################################################
    
tests = get_expected(LISTFILE)

if __name__ == "__main__":
    if (len(sys.argv) >= 2):
        if (sys.argv[1] == "all"):
            runallswsc()
            sys.exit(0)
        elif (sys.argv[1] == "one"):
            if (len(sys.argv) == 3):
                runoneswsc(sys.argv[2])
                sys.exit(0)
    
    print('Syntax error!')
    help_print()

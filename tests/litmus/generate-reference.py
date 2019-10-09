#!/usr/bin/python3

# Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
# Based on source code of Carl Leonardsson
# Run nidhugg "count-weak-trace" on all C tests.

import datetime
import subprocess
import sys
import tempfile
import os
import time

curDir = os.getcwd()
LITMUSDIR = curDir + '/C-tests'
OUTPUTTFILE = curDir + '/reference.results.txt'
LISTFILE = curDir + '/listfile.txt'
# FIX ME
NIDHUGGBIN = 'nidhuggc'




def get_expected(fname):
    f = open(fname,'r')
    l = []
    
    for ln in f:
        ln = ln.strip()
        if len(ln) and not(ln[0] == '#'):
            segments = ln.split('.')
            del segments[-1]
            tst = '.'.join(segments)
            l.append({'tstname':tst})
    f.close()

    return l





def res_to_string(tst):
    s = tst['tstname']+' '

    if not('nidhugg allow' in tst):
        s = s+'FAILURE: '
        if 'failure' in tst:
            s = s + tst['failure']
        else:
            s = s + '(unknown)'
        return s

    if tst['nidhugg allow']:
        s = s + 'Allow'
    else:
        s = s + 'Forbid'

    return s




def runallnidhugg():
    logfile = open(OUTPUTTFILE,'w')
    logfile.write('# Results of run nidhugg "count-weak-trace" on all C tests.\n')
    version = subprocess.check_output([NIDHUGGBIN, '--version'], stderr = subprocess.STDOUT).decode()
    logfile.write('# ' + version)
    logfile.write('# nidhuggc --sc TEST.c \n')
    logfile.write('# The tests where executed using test-nidhugg.py.\n')
    logfile.write('# Date: ' + datetime.datetime.now().strftime('%y%m%d-%H:%M') + '\n')
    logfile.write('\n')

    totaltracecount = 0
    tracecount = 0
    n = 0
    t0 = time.time()
    
    for tst in tests:
        n = n+1
        try:
            out = subprocess.check_output([NIDHUGGBIN, '--sc',
                                           LITMUSDIR + '/' + tst['tstname'] + '.c'],
                                          stderr = subprocess.STDOUT).decode()
            parts_0 = out.split("\n")
            error_info = parts_0[-6]
            trace_info = parts_0[-7]
            tracecount = int(trace_info.split(":")[-1])
            totaltracecount += tracecount
            if out.find('No errors were detected') >= 0:
                tst['nidhugg allow'] = False
            else:
                tst['nidhugg allow'] = True
        except subprocess.CalledProcessError:
            tst['failure'] = 'nidhugg'
            tracecount = -1
            continue
        finally:
            print('{0:4}: '.format(n), end = '')
            print(res_to_string(tst) + ' : ' + str(tracecount))
            logfile.write(res_to_string(tst) + ' : ' + str(tracecount) + '\n')
            
    runtime = time.time() - t0
    logfile.write('# Total number of traces: ' + str(totaltracecount) + '\n')
    logfile.write('# Total running time: {0:.2f} s\n'.format(runtime))
    logfile.close()





def runonenidhugg(filename):
    tracecount = 0
    found = False
    
    for tst in tests:
        if tst['tstname'] != filename:
            continue
        else:
            found = True
            try:
                out = subprocess.check_output([NIDHUGGBIN, '--sc',
                                               LITMUSDIR + '/' + tst['tstname'] + '.c'],
                                              stderr=subprocess.STDOUT).decode()
                parts_0 = out.split("\n")
                error_info = parts_0[-6]
                trace_info = parts_0[-7]
                tracecount = int(trace_info.split(":")[-1])
                if out.find('No errors were detected') >= 0:
                    tst['nidhugg allow'] = False
                else:
                    tst['nidhugg allow'] = True
            except subprocess.CalledProcessError:
                tst['failure'] = 'nidhugg'
                tracecount = -1
            finally:
                print(res_to_string(tst) + ' : ' + str(tracecount))
                break
            
    if found == False:
        print('Test case was not found!')





def help_print():
    print('Possible commands:')
    print('\t1. python3 ./generate-reference.py all')
    print('\t2. python3 ./generate-reference.py one testname(without_dot_c)')





####################################################
    
tests = get_expected(LISTFILE)

if __name__ == "__main__":
    if (len(sys.argv) >= 2):
        if (sys.argv[1] == "all"):
            runallnidhugg()
            sys.exit(0)
        elif (sys.argv[1] == "one"):
            if (len(sys.argv) == 3):
                runonenidhugg(sys.argv[2])
                sys.exit(0)
    print('Syntax error!')
    help_print()

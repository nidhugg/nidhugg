#!/usr/bin/python3

# Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
# Based on source code of Carl Leonardsson
# Run nidhuggc on all C tests.

import datetime
import subprocess
import sys
import tempfile
import os
import time
import argparse
import multiprocessing

curDir = os.getcwd()
LITMUSDIR = curDir + '/C-tests'
OUTPUTTFILE = curDir + '/test.results.txt'
LISTFILE = curDir + '/reference.results.txt'
# FIX ME
NIDHUGGCBIN = 'nidhuggc'

class bcolors:
    FAIL = '\033[91m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    ENDC = '\033[0m'


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


def res_to_string(tst, res):
    s = tst['tstname']
    
    if not('allow' in res):
        s = s + bcolors.FAIL + ' FAILURE: ' + bcolors.ENDC
        if 'failure' in tst:
            s = s + tst['failure']
        else:
            s = s + '(unknown)'
        return (s, False)

    if res['allow'] != tst['expect allow']:
        s = s + bcolors.FAIL + ' FAILURE: unexpected result ' + ('Allow' if res['allow'] else 'Forbid') + bcolors.ENDC
        return (s, False)
    else:
        s = s + ' ' + ('Allow' if res['allow'] else 'Forbid')

    if tst['expected trace count'] != res['tracecount']:
        return (s + bcolors.FAIL + ' FAILURE: expected ' + str(tst['expected trace count'])
                + ' not ' + str(res['tracecount']) + ' traces' + bcolors.ENDC, False)

    return (s + bcolors.OKGREEN + ' OK ' + bcolors.ENDC + ' : ' + str(res['tracecount']), True)

    
def runallnidhuggc(jobs, keep_going):
    logfile = open(OUTPUTTFILE, 'w')
    logfile.write('# Results of running nidhuggc to all C tests.\n')
    version = subprocess.check_output([NIDHUGGCBIN, '--version'], stderr = subprocess.STDOUT).decode()
    logfile.write('# ' + version)
    logfile.write('# nidhuggc --sc --rf TEST.c \n')
    logfile.write('# The tests where executed using test-nidhuggc.py.\n')
    logfile.write('# Date: ' + datetime.datetime.now().strftime('%y%m%d-%H:%M')+'\n')
    logfile.write('\n')

    ret = 0
    totaltracecount = 0
    tests = get_expected(LISTFILE)
    initial_count = len(tests)
    t0 = time.time()

    q = multiprocessing.Queue()
    workers = []
    work = dict()

    def give_work(w, p):
        if len(tests) > 0:
            tst = tests.pop()
            n = initial_count - len(tests)
            p.send((n,tst))
            work[n] = (tst,w,p)
        else:
            p.send('die')
            w.join()

    for _ in range(min(len(tests), jobs)):
        (o, i) = multiprocessing.Pipe(False)
        p = multiprocessing.Process(target=slave, args=(q, o))
        p.start()
        workers.append((p, i))
        give_work(p, i)

    while len(work) > 0:
        (done_n, res) = q.get()
        (tst,w,p) = work.pop(done_n)
        totaltracecount += res['tracecount']
        print('{0:4}: '.format(done_n), end = '')
        (s, ok) = res_to_string(tst, res)
        print(s)
        logfile.write(s  + '\n')
        if not ok:
            if not keep_going: tests.clear()
            ret = 1
        give_work(w, p)

    runtime = time.time() - t0
    logfile.write('# Total number of traces: ' + str(totaltracecount) + '\n')
    logfile.write('# Total running time: {0:.2f} s\n'.format(runtime))
    logfile.close()

    return ret

def slave(q, p):
    while True:
        cmd = p.recv()
        if cmd == 'die': break
        (n,tst) = cmd
        res = run_test(tst)
        q.put((n, res))

def run_test(tst):
    res = dict()
    try:
        srcfile = LITMUSDIR + '/' + tst['tstname'] + '.c'
        extra_opts = look_for_extra_opts(srcfile) or ['--sc', '--rf']
        out = subprocess.check_output([NIDHUGGCBIN]
                                      + extra_opts + [srcfile],
                                      stderr = subprocess.STDOUT).decode()
        lines = out.split("\n")
        res['tracecount'] = grep_count(lines, "Trace count: ") \
            + grep_count(lines, "Assume-blocked trace count: ") \
            + grep_count(lines, "Await-blocked trace count: ") \
            + grep_count(lines, "Sleepset-blocked trace count: ")
        if out.find('No errors were detected') >= 0:
            res['allow'] = False
        else:
            res['allow'] = True
    except subprocess.CalledProcessError:
        res['failure'] = NIDHUGGCBIN
        res['tracecount'] = -1
    return res

def look_for_extra_opts(srcfile):
    with open(srcfile, 'r', encoding='UTF-8') as f:
        while True:
            line = f.readline()
            if not line: break
            if not line.startswith('//'): break
            if line.startswith('// nidhuggc:'):
                return list(filter(None, line[12:].strip().split(' ')))
        return []

def grep_count(lines, start):
    """
    Finds the first line that starts with `start` and returns the
    integer that follows. Otherwise, returns 0.
    """
    for line in lines:
        if line.startswith(start):
            return int(line[len(start):])
    return 0

def runonenidhuggc(filename):
    for tst in get_expected(LISTFILE):
        if tst['tstname'] == filename:
            res = run_test(tst)
            (s, ok) = res_to_string(tst, res)
            print(s)
            return 0 if ok else 1

    print('Test case was not found!')
    return 2

####################################################

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    # How many threads we want (default, number of CPUs in the system)
    threads = os.getenv("THREADS", "")
    if threads == "":
        try:
            threads = str(multiprocessing.cpu_count())
        except:
            threads = "1"
    parser.add_argument('-j', dest='jobs', metavar='N', type=int,
                        default=int(threads), help='Number or parallel jobs')
    parser.add_argument('-k', '--keep-going', dest='keep_going',
                        action='store_true',
                        help='Run all tests, even if one fails')
    parser.add_argument('--nidhuggc', dest='nidhuggc', type=str,
                        default=NIDHUGGCBIN,
                        help='Nidhugg binary to use')
    modes = parser.add_subparsers(metavar='mode',dest='mode')
    all_parser = modes.add_parser('all', help='Run all tests')
    one_parser = modes.add_parser('one', help='Run single test')
    one_parser.add_argument('test', help='testname(without_dot_c)');
    args = parser.parse_args()
    NIDHUGGCBIN = args.nidhuggc
    if (args.mode == "all"):
        ret = runallnidhuggc(args.jobs, args.keep_going)
        sys.exit(ret)
    elif (args.mode == "one"):
        ret = runonenidhuggc(args.test)
        sys.exit(ret)
    else:
        parser.print_help()

#!%%PYTHON%%

import atexit
import os
import subprocess
import sys
import tempfile
import time

#########################
# Default Configuration #
#########################

NIDHUGG='%%NIDHUGGPATH%%'
CLANG='%%CLANG%%'
CLANGXX='%%CLANGXX%%'

nidhuggcparams = [
    {'name':'--help','help':'Prints this text.','param':False},
    {'name':'--version','help':'Prints the nidhugg version.','param':False},
    {'name':'--c','help':'Interpret input FILE as C code. (Compile with clang.)','param':False},
    {'name':'--cxx','help':'Interpret input FILE as C++ code. (Compile with clang++.)','param':False},
    {'name':'--clang','help':'Specify the path to clang.','param':'PATH'},
    {'name':'--clangxx','help':'Specify the path to clang++.','param':'PATH'},
    {'name':'--nidhugg','help':'Specify the path to the nidhugg binary.','param':'PATH'},
    {'name':'--no-spin-assume','help':'Don\'t use the spin-assume transformation on module before calling nidhugg.','param':False},
    {'name':'--unroll','help':'Use unroll transformation on module before calling nidhugg.','param':'N'}
]

nidhuggcparamaliases = {
    '-help':'--help',
    '-h':'--help',
    '-?':'--help',
    '-version':'--version',
    '-V':'--version',
    '-c':'--c',
    '-cxx':'--cxx',
    '-clang':'--clang',
    '-clangxx':'--clangxx',
    '-nidhugg':'--nidhugg',
    '-no-spin-assume':'--no-spin-assume',
    '-unroll':'--unroll'
}

# The name (absolute path) of the temporary directory where all
# temporary files will be created.
tmpdir=None

def init_tmpdir():
    global tmpdir
    if tmpdir == None:
        tmpdir=tempfile.mkdtemp()

def destroy_tmpdir():
    if tmpdir != None:
        files = os.listdir(tmpdir)
        for f in files:
            os.remove(os.path.join(tmpdir,f))
        os.rmdir(tmpdir)

atexit.register(destroy_tmpdir)

def run(cmd,ignoreret=False):
    return_codes = [0, 42]
    cmdstr=''
    for s in cmd:
        cmdstr = cmdstr+('' if cmdstr=='' else ' ')+s
    print("* Nidhuggc: $ "+cmdstr)
    retval = subprocess.call(cmd)
    if not(ignoreret) and retval not in return_codes:
        raise Exception('Command ({1}) returned an error exit code ({0}).'.format(retval,cmd[0]))
    return retval

# Read arguments from sys.argv. Return a tuple (A,B,C) of disjunct
# argument lists A, B, C, together containing precisely the arguments
# in sys.argv:
# - A: Arguments targeted to nidhuggc as a dict switch=>(parameter or empty string)
# - B: Arguments targeted to the compiler
# - C: Arguments targeted to nidhugg
#
# If an input file FILE is specified in sys.argv, then
# '--input'=>FILE is also included in A.
def get_args():
    A = {}
    B = []
    C = []
    disablesinput=['--help','--version']
    def canonize(arg):
        if arg in nidhuggcparamaliases:
            return nidhuggcparamaliases[arg]
        i = arg.find('=')
        if 0 <= i and (arg[:i] in nidhuggcparamaliases):
            return nidhuggcparamaliases[arg[:i]]+arg[i:]
        return arg
    shouldhaveinput=(0 == len([a for a in sys.argv[1:] if canonize(a) in disablesinput]))
    fornidhuggcsingle=[p['name'] for p in nidhuggcparams if p['param'] == False]
    hascompilerargs=False
    i = 1
    argc = len(sys.argv)
    if shouldhaveinput:
        if argc<2: print_help(); raise Exception('No input file specified.')
        A['--input'] = sys.argv[argc-1]
        argc = argc-1
    while i < argc:
        arg = canonize(sys.argv[i])
        i = i+1
        if arg in fornidhuggcsingle:
            A[arg]=''
        else:
            foundparam=False
            for ap in nidhuggcparams:
                if ap['param'] == False: continue
                if arg.startswith(ap['name']+'='):
                    A[ap['name']]=arg[len(ap['name'])+1:]
                    foundparam=True
                    break
                elif arg == ap['name']:
                    if i == argc: print_help(); raise Exception('Switch '+ap['name']+' requires an argument.')
                    A[ap['name']]=sys.argv[i]
                    i = i+1 # Ignore the next argument
                    foundparam=True
                    break
            if foundparam: pass
            elif arg == '--' and not(hascompilerargs):
                B = C
                C = []
                hascompilerargs=True
            else:
                C.append(arg)
    return (A,B,C)

def help_indent(s,n):
    ind=' '*n
    return ind+(s.replace('\n','\n'+ind))

def print_help():
    print('Usage: {0} [[COMPILER/NIDHUGGC OPTIONS --] NIDHUGG/NIDHUGGC OPTIONS] FILE'.format(sys.argv[0]))
    print('')
    print(' - FILE should be a source code file in C or C++.')
    print(' - COMPILER OPTIONS are options that will be sent to the compiler')
    print('   (clang/clang++).')
    print(' - NIDHUGG OPTIONS are options that will be sent to nidhugg.')
    print('   (See nidhugg --help for details.)')
    print('')
    print('NIDHUGGC OPTIONS:')
    for p in nidhuggcparams:
        if p['param'] == False:
            print('  {0}\n{1}'.format(p['name'],help_indent(p['help'],6)))
        else:
            print('  {0}={1}\n{2}'.format(p['name'],p['param'],help_indent(p['help'],6)))

def print_version():
    subprocess.call([NIDHUGG,'-version'])

def get_lang(nidhuggcargs):
    c = 'C'
    cxx = 'C++'
    if not('--input' in nidhuggcargs):
        print_help()
        raise Exception('No input file specified.')
    if '--c' in nidhuggcargs:
        if '--cxx' in nidhuggcargs:
            raise Exception('Contradicting switches --c and --cxx were given.')
        return c
    if '--cxx' in nidhuggcargs:
        return cxx
    fname = nidhuggcargs['--input']
    if fname.endswith('.c') or fname.endswith('.C'):
        return c
    if len(fname) >= 4 and fname[-4:] in ['.cpp','.cxx','.c++','.C++','.hpp','.tcc']:
        return cxx
    print("WARNING: Unable to infer the input language for input source code.")
    print("WARNING: Guessing C.")
    return c

# Compile the input source code file to LLVM IR in a temporary
# file. Return the path to the temporary file.
def get_IR(nidhuggcargs,compilerargs):
    if not('--input' in nidhuggcargs):
        print_help()
        raise Exception('No input file specified.')
    init_tmpdir()
    inputfname = nidhuggcargs['--input']
    (fd,outputfname) = tempfile.mkstemp(suffix='.ll',dir=tmpdir)
    os.close(fd)
    lang=get_lang(nidhuggcargs)
    if lang == 'C':
        cmd = [CLANG,'-o',outputfname,'-S','-emit-llvm','-g']
        cmd.extend(compilerargs)
        cmd.append(inputfname)
        run(cmd)
    else:
        assert(lang == 'C++')
        cmd = [CLANGXX,'-o',outputfname,'-S','-emit-llvm','-g']
        cmd.extend(compilerargs)
        cmd.append(inputfname)
        run(cmd)
    return outputfname

def transform(nidhuggcargs,transformargs,irfname):
    (fd,outputfname) = tempfile.mkstemp(suffix='.ll',dir=tmpdir)
    os.close(fd)
    cmd = [NIDHUGG]+transformargs+['-transform',outputfname,irfname]
    run(cmd)
    return outputfname

def run_nidhugg(nidhuggcargs,nidhuggargs,irfname):
    cmd = [NIDHUGG]+nidhuggargs+[irfname]
    return run(cmd)

def main():
    try:
        global CLANG
        global CLANGXX
        global NIDHUGG
        t0 = time.time()
        (nidhuggcargs,compilerargs,nidhuggargs) = get_args()
        transformargs=[]
        for argname in nidhuggcargs:
            argarg = nidhuggcargs[argname]
            if argname == '--help':
                print_help()
                exit(0)
            elif argname == '--clang':
                CLANG=argarg
            elif argname == '--clangxx':
                CLANGXX=argarg
            elif argname == '--nidhugg':
                NIDHUGG=argarg
            elif argname == '--no-spin-assume':
                transformargs.append(argname)
            elif argname == '--unroll':
                transformargs.append('--unroll={0}'.format(argarg))
        if '--version' in nidhuggcargs:
            # Wait with printing version until all arguments have been parsed.
            # Because the nidhugg binary may be specified after --version.
            print_version()
            exit(0)
        if not(os.path.exists(nidhuggcargs['--input'])):
            print_help()
            raise Exception('Source code file \'{0}\' does not exist.'.format(nidhuggcargs['--input']))
        # Compile
        irfname = get_IR(nidhuggcargs,compilerargs)
        # Transform
        irfname = transform(nidhuggcargs,transformargs,irfname)
        # Run stateless model-checker
        ret = run_nidhugg(nidhuggcargs,nidhuggargs,irfname)
        print('Total wall-clock time: {0:.2f} s'.format(time.time()-t0))
        exit(ret)
    except Exception as e:
        print("\nNidhuggc Error: {0}".format(str(e)))
        exit(1)

if __name__ == '__main__':
    main()

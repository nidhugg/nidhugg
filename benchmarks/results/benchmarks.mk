SHELL = /bin/bash -o pipefail

TIME_LIMIT ?= 60 # seconds
STACK_LIMIT ?= 65536 # kB
MEM_LIMIT ?= 8388608 # kB

SRCDIR = ../../..
CLANG = clang
CC = gcc
OPT =
CLANGFLAGS = -c -emit-llvm $(OPT)
NIDHUGG ?= nidhugg
SOURCE    = $(NIDHUGG) -c11 -sc -source
OPTIMAL   = $(NIDHUGG) -c11 -sc -optimal
OBSERVERS = $(NIDHUGG) -c11 -sc -observers
RFSC_THREADS = 1
RFSC      = $(NIDHUGG) -c11 -sc -rf -n-threads=$(RFSC_THREADS)
DCDPOR ?= dcdpor -dc
RCMC ?= rcmc
WRCMC ?= $(RCMC) --wrc11
CDSC_DIR ?= /opt/cdschecker
TIME = env time -f 'real %e\nres %M'
TIMEOUT = timeout $(TIME_LIMIT)
ULIMIT = ulimit -Ss $(STACK_LIMIT) && ulimit -Sv $(MEM_LIMIT) &&
RUN = -$(ULIMIT) $(TIMEOUT) $(TIME)
TABULATE = ../../tabulate.sh

TOOLS = optimal observers rfsc dcdpor rcmc wrcmc
ifneq ($(wildcard $(CDSC_DIR)/.),)
        TOOLS += cdsc
else
	NATOOLS += cdsc
endif

TABLES = $(TOOLS:%=%.txt) wide.txt
ALL_RESULTS = $(TOOLS:%=%_results)
BITCODE_FILES = $(N:%=code_%.bc)
# Add the bitcode files as explicit targets, otherwise Make deletes them after
# benchmark, and thus reruns *all* benchmarks of a particular size if any of
# them need to be remade
all: $(TABLES) $(BITCODE_FILES)

# Hack to not duplicate tabulation rule; causes the table to be rebuilt on each
# invocation
rcmc_results:: $(N:%=rcmc_%.txt)
wrcmc_results:: $(N:%=wrcmc_%.txt)
source_results:: $(N:%=source_%.txt)
optimal_results:: $(N:%=optimal_%.txt)
observers_results:: $(N:%=observers_%.txt)
rfsc_results:: $(N:%=rfsc_%.txt)
dcdpor_results:: $(N:%=dcdpor_%.txt)
cdsc_results:: $(N:%=cdsc_%.txt)

code_%.bc: $(SRCDIR)/$(FILE)
	$(CLANG) -DN=$* $(CLANGFLAGS) -o $@ $<

cdsc_%.elf: $(SRCDIR)/$(FILE)
	$(CC) -Wl,-rpath=$(CDSC_DIR) $(OPT) -I../../cdsc_include \
		-L$(CDSC_DIR) -lmodel -I$(CDSC_DIR)/include \
		../../cdsc_lib/mutex.cpp -lstdc++ \
		-DCDSC=1 -Dmain=user_main -pthread -DN=$* -o $@ $<

source_%.txt: code_%.bc
	@date
	$(RUN) $(SOURCE) $< 2>&1 | tee $@

optimal_%.txt: code_%.bc
	@date
	$(RUN) $(OPTIMAL) $< 2>&1 | tee $@

observers_%.txt: code_%.bc
	@date
	$(RUN) $(OBSERVERS) $< 2>&1 | tee $@

rfsc_%.txt: code_%.bc
	@date
	$(RUN) $(RFSC) $< 2>&1 | tee $@

dcdpor_%.txt: code_%.bc
	@date
	$(RUN) $(DCDPOR) $< 2>&1 | tee $@

rcmc_%.txt: code_%.bc
	@date
	$(RUN) $(RCMC) $< 2>&1 | tee $@

wrcmc_%.txt: code_%.bc
	@date
	$(RUN) $(WRCMC) $< 2>&1 | tee $@

cdsc_%.txt: cdsc_%.elf
	@date
	$(RUN) ./$< 2>&1 | tee $@

%.txt: %_results $(TABULATE)
	$(TABULATE) tool "$(NAME)" $* "$(N)" \
	  | column -t > $@

wide.txt: $(ALL_RESULTS) $(TABULATE)
	$(TABULATE) wide "$(NAME)" "$(TOOLS)" "$(N)" "$(NATOOLS)" \
	  | column -t > $@

clean:
	rm -f *.bc *.elf $(TABLES)
mrproper: clean
	rm -f *.txt

.PHONY: clean all benchmark mrproper # $(TOOLS:%=%_results)
.DELETE_ON_ERROR:

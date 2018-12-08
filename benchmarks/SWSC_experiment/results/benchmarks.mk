SHELL = /bin/bash -o pipefail

TIME_LIMIT ?= 60 # seconds
STACK_LIMIT ?= 65536 # kB

SRCDIR = ../../../../..
CLANG = clang
CC = gcc
OPT = -O2
CLANGFLAGS = -c -emit-llvm $(OPT)
NIDHUGG ?= nidhugg
SOURCE = $(NIDHUGG) -sc
OPTIMAL = $(NIDHUGG) -sc -optimal
OBSERVERS = $(NIDHUGG) -sc -optimal -observers
SWSC ?= /home/magnus/swsc/build_art/src/swsc
RCMC ?= rcmc
WRCMC ?= $(RCMC) --wrc11
CDSC_DIR ?= /opt/cdschecker
TIME = env time -f 'real %e\nres %M'
TIMEOUT = timeout $(TIME_LIMIT)
ULIMIT = ulimit -Ss $(STACK_LIMIT) &&
RUN = -$(ULIMIT) $(TIMEOUT) $(TIME)
TABULATE = ../../../../tabulate.sh

TOOLS = observers swsc rcmc wrcmc
# ifneq ($(wildcard $(CDSC_DIR)/.),)
#        TOOLS += cdsc
# else
#	NATOOLS += cdsc
# endif

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
swsc_results:: $(N:%=swsc_%.txt)
cdsc_results:: $(N:%=cdsc_%.txt)

code_%.bc: $(SRCDIR)/$(FILE)
	$(CLANG) -DN=$* $(CLANGFLAGS) -o $@ $<

cdsc_%.elf: $(SRCDIR)/$(FILE)
	$(CLANG) -DN=$* $(CLANGFLAGS) -o $@ $<

cdsc_%.elf: $(SRCDIR)/$(FILE)
	$(CC) -Wl,-rpath=$(CDSC_DIR) $(OPT) -I../../cdsc_include \
		-L$(CDSC_DIR) -lmodel -I$(CDSC_DIR)/include \
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

swsc_%.txt: code_%.bc
	@date
	$(RUN) $(SWSC) $< 2>&1 | tee $@

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

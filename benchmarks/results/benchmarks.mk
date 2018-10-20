SHELL = /bin/bash -o pipefail

TIME_LIMIT ?= 60 # seconds

SRCDIR = ../../..
CLANG = clang
CC = gcc
OPT = -O2
CLANGFLAGS = -c -emit-llvm $(OPT)
NIDHUGG ?= nidhugg
SOURCE = $(NIDHUGG) -sc
OPTIMAL = $(NIDHUGG) -sc -optimal
OBSERVERS = $(NIDHUGG) -sc -optimal -observers
SWSC ?= swsc
RCMC ?= rcmc
CDSC_DIR ?= /opt/cdschecker
TIME = env time -f 'real %e\nres %M'
TIMEOUT = timeout $(TIME_LIMIT)
RUN = -$(TIMEOUT) $(TIME)
TABULATE = ../../tabulate.sh

TOOLS = source optimal observers swsc rcmc # cdsc
TABLES = $(TOOLS:%=%.txt)
all: $(TABLES)

# Hack to not duplicate tabulation rule; causes the table to be rebuilt on each
# invocation
rcmc_results:: $(N:%=rcmc_%.txt)
source_results:: $(N:%=source_%.txt)
optimal_results:: $(N:%=optimal_%.txt)
observers_results:: $(N:%=observers_%.txt)
swsc_results:: $(N:%=swsc_%.txt)
cdsc_results:: $(N:%=cdsc_%.txt)

code_%.bc: $(SRCDIR)/$(FILE)
	$(CLANG) -DN=$* $(CLANGFLAGS) -o $@ $<

cdsc_%.elf: $(SRCDIR)/$(FILE)
	$(CC) -Wl,-rpath=$(CDSC_DIR) $(OPT) \
		-L$(CDSC_DIR) -lmodel -I$(CDSC_DIR)/include \
		-Dmain=user_main -pthread -DN=$* -o $@ $<

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

cdsc_%.txt: cdsc_%.elf
	@date
	$(RUN) ./$< 2>&1 | tee $@

%.txt: %_results $(TABULATE)
	$(TABULATE) "$(NAME)" $* "$(N)" \
	  | column -t > $@

clean:
	rm -f *.bc *.elf $(TABLES)
mrproper: clean
	rm -f *.txt

.PHONY: clean all benchmark mrproper # $(TOOLS:%=%_results)
.DELETE_ON_ERROR:

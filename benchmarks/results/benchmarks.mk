SHELL = /bin/bash -o pipefail

TIME_LIMIT ?= 36000 # seconds
STACK_LIMIT ?= 65536 # kB
MEM_LIMIT ?= 33554432 # kB

SRCDIR = ../../..
CLANG = clang
CC = gcc
OPT = # -O1
TOOLCLANGFLAGS = $(OPT)
OPTFLAGS = -mem2reg -deadargelim
CLANGFLAGS = -c -emit-llvm -Xclang -disable-O0-optnone $(TOOLCLANGFLAGS)
NIDHUGG ?= nidhugg
SOURCE    = $(NIDHUGG) -c11 -sc -source
OPTIMAL   = $(NIDHUGG) -c11 -sc -optimal
OBSERVERS = $(NIDHUGG) -c11 -sc -observers
RFSC      = $(NIDHUGG) -c11 -sc -rf --n-threads=$(1)
DCDPOR ?= dcdpor -dc
VCDPOR ?= vcdpor -vc
RCMC ?= rcmc
WRCMC ?= $(RCMC) --wrc11
GENMC ?= genmc -input-from-bitcode-file
CDSC_DIR ?= /opt/cdschecker
TIME = env time -f 'real %e\nres %M'
TIMEOUT = timeout $(TIME_LIMIT)
ULIMIT = ulimit -Ss $(STACK_LIMIT) && ulimit -Sv $(MEM_LIMIT) &&
RUN = -$(ULIMIT) $(TIMEOUT) $(TIME)
TABULATE = ../../tabulate.sh

TOOLS = genmc rfsc rfsc32 vcdpor
ifneq ($(wildcard $(CDSC_DIR)/.),)
        TOOLS += cdsc
else
	NATOOLS += cdsc
endif
# SPECIAL_TOOLS are tools that need custom measurement rules, the default ones
# will not be generated
SPECIAL_TOOLS = cdsc rfsc32
THREADS = 1
rfsc_THREADS = 1 2 4 8 16 24 32 48 64 # 96
# TOOLS=rfsc
# THREADS = 1 2 4 8 16 24 32 48 64

TABLES = $(TOOLS:%=%.txt) wide.txt
# Only for wide.txt (not including $(tool)_THREADS
ALL_RESULTS = $(foreach tool,$(TOOLS),$(foreach n,$(N),\
	$(foreach t,$(THREADS),$(tool)_$(n)_$(t).txt)))
BITCODE_FILES = $(N:%=code_%.bc)
# Add the bitcode files as explicit targets, otherwise Make deletes them after
# benchmark, and thus reruns *all* benchmarks of a particular size if any of
# them need to be remade
all: $(TABLES) $(BITCODE_FILES)

code_%.bc: $(SRCDIR)/$(FILE)
	$(CLANG) -DN=$* $(CLANGFLAGS) -o - $< | opt $(OPTFLAGS) -o $@

cdsc_%.elf: $(SRCDIR)/$(FILE)
	$(CC) -Wl,-rpath=$(CDSC_DIR) $(OPT) -I../../cdsc_include \
		-L$(CDSC_DIR) -lmodel -I$(CDSC_DIR)/include \
		../../cdsc_lib/mutex.cpp -lstdc++ \
		-DCDSC=1 -Dmain=user_main -pthread -DN=$* -o $@ $<

define tool_template =
 $(1)_$(3)_$(2).txt: code_$(3).bc
	@date
	$$(RUN) $$(call $(shell echo $(1) | tr a-z A-Z),$(2)) $$< 2>&1 | tee $$@
 $(1).txt: $(1)_$(3)_$(2).txt
endef

$(foreach tool,$(filter-out $(SPECIAL_TOOLS),$(TOOLS)),\
	$(foreach t,$(or $($(tool)_THREADS), $(THREADS)),\
		$(foreach n,$(N),\
			$(eval $(call tool_template,$(tool),$(t),$(n))))))

cdsc_%_1.txt: cdsc_%.elf
	@date
	$(RUN) ./$< 2>&1 | tee $@
cdsc.txt: $(foreach n,$(N),cdsc_$(n)_1.txt)

rfsc32_%_1.txt: rfsc_%_32.txt
	cp $< $@
rfsc32.txt: $(foreach n,$(N),rfsc32_$(n)_1.txt)

define tool_tab_template =
$(1).txt: $$(TABULATE)
	$$(TABULATE) tool "$$(NAME)" $(1) "$$(N)" "$$(or $$($(1)_THREADS), $$(THREADS))" \
	  | column -t > $(1).txt
endef

$(foreach tool,$(TOOLS),\
	$(eval $(call tool_tab_template,$(tool))))

wide.txt: $(ALL_RESULTS) $(TABULATE)
	$(TABULATE) wide "$(NAME)" "$(TOOLS)" "$(N)" "$(THREADS)" "$(NATOOLS)" \
	  | column -t > $@

clean:
	rm -f *.bc *.elf $(TABLES)
mrproper: clean
	rm -f *.txt

.PHONY: clean all benchmark mrproper
.DELETE_ON_ERROR:

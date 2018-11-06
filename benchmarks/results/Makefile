	# from_DCDPOR/parker		\ # not the same benchmark
SUBDIRS =				\
	from_DCDPOR/opt_lock		\
	from_OBSERVERS/fib_bench	\
	from_OBSERVERS/floating_read	\
	from_OBSERVERS/lamport		\
	from_OBSERVERS/lastwrite	\
	synthetic/length_parametric	\
	synthetic/race_parametric	\
	synthetic/thread_parametric

# T ?= all

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(T)

mrproper:
	for d in $(SUBDIRS); do \
		$(MAKE) -C$$d mrproper; \
	done


.PHONY: $(SUBDIRS) all mrproper

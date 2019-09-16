SUBDIRS =				\
	from_SCTBench/circular_buffer   \
        from_CONCUERROR/lastzero        \
        from_RCMC/readers               \
        from_RCMC/casrot                \
        from_OBSERVERS/fib_bench        \
	from_OBSERVERS/lastwrite 	\
	SV-COMP/sigma			\
	from_OBSERVERS/floating_read 	\
	from_TRACER/control_flow 	\
	from_DCDPOR/opt_lock		\
        synthetic/race_parametric       \
        from_TRACER/exponential_bug     \
        from_DCDPOR/parker              \
        from_OBSERVERS/lamport          \
        other/approxds_append

# T ?= all

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(T)

mrproper:
	for d in $(SUBDIRS); do \
		$(MAKE) -C$$d mrproper; \
	done

.PHONY: $(SUBDIRS) all mrproper

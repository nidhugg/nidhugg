SUBDIRS =				\
        from_CONCUERROR/lastzero        \
        from_RCMC/casrot                \
        from_OBSERVERS/fib_bench        \
	SV-COMP/sigma			\
	from_TRACER/control_flow 	\
	from_DCDPOR/opt_lock		\
        synthetic/race_parametric       \
        other/approxds_append
#	from_OBSERVERS/lamport          \
#	from_DCDPOR/parker              \
#	from_TRACER/exponential_bug     \
#	from_OBSERVERS/floating_read 	\
#	from_OBSERVERS/lastwrite 	\
#	from_SCTBench/circular_buffer   \
#	from_RCMC/readers               \

# T ?= all

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(T)

mrproper:
	for d in $(SUBDIRS); do \
		$(MAKE) -C$$d mrproper; \
	done

.PHONY: $(SUBDIRS) all mrproper

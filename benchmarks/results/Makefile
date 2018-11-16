SUBDIRS =				\
	from_CONCUERROR/lastzero	\
	from_DCDPOR/opt_lock		\
	from_OBSERVERS/fib_bench	\
	from_OBSERVERS/floating_read	\
	from_OBSERVERS/lamport		\
	from_OBSERVERS/lastwrite	\
	from_TRACER/exponential_bug	\
	from_TRACER/redundant_co	\
	from_TRACER/myexample		\
	from_TRACER/control_flow	\
	SV-COMP/sigma			\
	synthetic/sat_example		\
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

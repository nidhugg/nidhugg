 SUBDIRS =  benchmarks/asserts/from_DCDPOR/parker		\
	benchmarks/asserts/from_OBSERVERS/floating_read	\
	benchmarks/asserts/from_OBSERVERS/fib_bench		\
	benchmarks/asserts/from_SATCHECK/dekker			\
	benchmarks/asserts/from_SATCHECK/linuxlock			\
	benchmarks/asserts/from_SATCHECK/linuxrwlock		\
	benchmarks/asserts/from_TRACER/bakery				\
	benchmarks/asserts/from_TRACER/burns				\
	benchmarks/asserts/from_TRACER/CO-2+2W				\
	benchmarks/asserts/from_TRACER/dijkstra			\
	benchmarks/asserts/from_TRACER/peterson			\
	benchmarks/asserts/from_TRACER/pgsql				\
	benchmarks/asserts/from_DPU/dispatcher				\
	benchmarks/asserts/from_DPU/poke					\
	benchmarks/asserts/from_DPU/multiprodcon			\
	benchmarks/asserts/SV-COMP/indexer					\
	benchmarks/asserts/SV-COMP/lamport					\
	benchmarks/asserts/SV-COMP/szymanski				\
	benchmarks/asserts/SV-COMP/queue_ok				\
	benchmarks/asserts/SV-COMP/stack_true			\
	benchmarks/noasserts/from_CONCUERROR/lastzero		\
	benchmarks/noasserts/from_DCDPOR/opt_lock			\
	benchmarks/noasserts/from_OBSERVERS/lastwrite		\
	benchmarks/noasserts/from_RCMC/readers				\
	benchmarks/noasserts/from_TRACER/filesystem			\
	benchmarks/noasserts/from_TRACER/control_flow		\
	benchmarks/noasserts/from_TRACER/myexample			\
	benchmarks/noasserts/from_TRACER/redundant_co		\
	benchmarks/noasserts/SV-COMP/pthread_demo			\
	benchmarks/noasserts/SV-COMP/sigma					\
	benchmarks/noasserts/from_MCR/account				\
	benchmarks/noasserts/from_MCR/airline				\
	benchmarks/noasserts/from_TRACER/alpha1			\
	benchmarks/noasserts/from_TRACER/alpha2			\
	benchmarks/noasserts/from_TRACER/CO-MP				\
	benchmarks/noasserts/from_TRACER/CO-R				\
	benchmarks/noasserts/from_TRACER/CO-S				\
	benchmarks/noasserts/from_TRACER/co1				\
	benchmarks/noasserts/from_TRACER/co4				\
	benchmarks/noasserts/from_TRACER/co10				\
	benchmarks/noasserts/from_TRACER/exponential_bug



# 	benchmarks/asserts/SV-COMP/workstealqueue_mutex_true_unreach_call  
#	benchmarks/noasserts/from_MCR/bubblesort			\



# SUBDIRS =															\
# 	benchmarks/noasserts/from_CONCUERROR/lastzero		\
# 	benchmarks/noasserts/from_DCDPOR/opt_lock			\
# 	benchmarks/noasserts/from_OBSERVERS/lastwrite		\
# 	benchmarks/noasserts/from_RCMC/readers				\
# 	benchmarks/noasserts/from_TRACER/filesystem			\
# 	benchmarks/noasserts/from_TRACER/control_flow		\
# 	benchmarks/noasserts/from_TRACER/myexample			\
# 	benchmarks/noasserts/from_TRACER/redundant_co		\
# 	benchmarks/noasserts/SV-COMP/pthread_demo			\
# 	benchmarks/noasserts/SV-COMP/sigma					\
# 	benchmarks/noasserts/from_MCR/account				\
# 	benchmarks/noasserts/from_MCR/airline				\
# 	benchmarks/noasserts/from_MCR/bubblesort			\
# 	benchmarks/noasserts/from_TRACER/alpha1			\
# 	benchmarks/noasserts/from_TRACER/alpha2			\
# 	benchmarks/noasserts/from_TRACER/CO-MP				\
# 	benchmarks/noasserts/from_TRACER/CO-R				\
# 	benchmarks/noasserts/from_TRACER/CO-S				\
# 	benchmarks/noasserts/from_TRACER/co1				\
# 	benchmarks/noasserts/from_TRACER/co4				\
# 	benchmarks/noasserts/from_TRACER/co10				\
# 	benchmarks/noasserts/from_TRACER/exponential_bug




# T ?= all

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(T)

mrproper:
	for d in $(SUBDIRS); do \
		$(MAKE) -C$$d mrproper; \
	done


.PHONY: $(SUBDIRS) all mrproper

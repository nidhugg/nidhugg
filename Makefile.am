SUBDIRS = src doc
dist_doc_DATA = README

.PHONY: test valtest
check:
	cd $(top_builddir)/tests &&	make check

test:
	cd $(top_builddir)/tests &&	make test

valtest:
	cd $(top_builddir)/tests &&	make valtest

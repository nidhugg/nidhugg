SUBDIRS = src doc
dist_doc_DATA = README

.PHONY: test valtest litmus mustpass
test:
	cd $(top_builddir)/src && $(MAKE) $(AM_MAKEFLAGS) test

valtest:
	cd $(top_builddir)/src && $(MAKE) $(AM_MAKEFLAGS) valtest

litmus_test:
	cd $(top_builddir)/src && $(MAKE) $(AM_MAKEFLAGS) litmus_test

smoke_test:
	cd $(top_builddir)/src && $(MAKE) $(AM_MAKEFLAGS) smoke_test

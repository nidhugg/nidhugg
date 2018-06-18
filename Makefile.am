SUBDIRS = src doc
dist_doc_DATA = README

.PHONY: test valtest litmus mustpass
test:
	cd $(top_builddir)/src && $(MAKE) $(AM_MAKEFLAGS) test

valtest:
	cd $(top_builddir)/src && $(MAKE) $(AM_MAKEFLAGS) valtest

litmus: 
	cd $(top_builddir)/src && $(MAKE) $(AM_MAKEFLAGS) litmus

mustpass:
	cd $(top_builddir)/src && $(MAKE) $(AM_MAKEFLAGS) mustpass

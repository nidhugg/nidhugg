SUBDIRS = src doc
dist_doc_DATA = README

.PHONY: test valtest
test:
	cd $(top_builddir)/src && $(MAKE) $(AM_MAKEFLAGS) test

valtest:
	cd $(top_builddir)/src && $(MAKE) $(AM_MAKEFLAGS) valtest

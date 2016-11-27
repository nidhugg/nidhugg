nobase_dist_doc_DATA = \
	manual.pdf

EXTRA_DIST = manual.tex

## By default, don't build the documentation, just use the prebuilt
## pdf as is from the repository.
## This way we avoid forcing users to install LaTeX dependencies.
## To (re)build the pdf, configure with the
## --enable-build-documentation switch.
if BUILDPDFDOCUMENTATION

manual.pdf: manual.tex
	cat manual.tex | \
	sed 's/%%NIDHUGGVERSION%%/@VERSION@/g' | \
	pdflatex -jobname manual || :
	cat manual.tex | \
	sed 's/%%NIDHUGGVERSION%%/@VERSION@/g' | \
	pdflatex -jobname manual || :
	cat manual.tex | \
	sed 's/%%NIDHUGGVERSION%%/@VERSION@/g' | \
	pdflatex -jobname manual || \
	echo -e '\n\n * Failed to build documentation.\n'

endif

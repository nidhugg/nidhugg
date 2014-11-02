nobase_dist_doc_DATA = \
	manual.pdf

EXTRA_DIST = manual.tex

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

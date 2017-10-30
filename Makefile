LATEX 	= latex -shell-escape
BIBTEX 	= bibtex
DVIPS 	= dvips
DVIPDF 	= dvipdft
XDVI	= xdvi -gamma 4
GH 		= gv

EXAMPLES	= $(wildcard *.c)
SRC	  := $(shell egrep -l '^[^%]*\\begin\{document\}' *.tex)
TRG 		= $(SRC:%.tex=%.dvi)
PSF 		= $(SRC:%.tex=%.ps)
PDF 		= $(SRC:%.tex=%.pdf)

TARGETS = noop-to-look-iosched
CC_C = $(CROSS_TOOL)gcc
CFLAGS = -Wall

all: pdf ps $(TARGETS)

pdf: $(PDF)

ps: $(PSF)

$(TRG): %.dvi: %.tex $(EXAMPLES)
	#use pygments to include source code
	#pygmentize -f latex -o __${EXAMPLES}.tex ${EXAMPLES}

	$(LATEX) $<
	$(BIBTEX) $(<:%.tex=%)
	$(LATEX) $<
	$(LATEX) $<
	#rm __${SRC}.tex
	#remove pygmentized output to avoid cluttering up directory

$(PSF):%.ps: %.dvi
	$(DVIPS) -R -Poutline -t letter $< -o $@

$(PDF): %.pdf: %.ps
	ps2pdf $<

show: $(TRG)
	@for i in $(TRG) ; do $(XDVI) $$i & done

showps: $(PSF)
	@for i in $(PSF) ; do $(GH) $$i & done

$(TARGETS):
	$(CC_C) $(CFLAGS) noop-to-look-iosched.c -o noop-to-look-iosched

clean:
	$(RM) noop-to-look-iosched
	rm -rf noop-to-look-iosched.dSYM
	rm -f *.pdf *.ps *.dvi *.out *.log *.aux *.bbl *.blg *.pyg

.PHONY: all show clean ps pdf showps

LATEX 	= latex -shell-escape
BIBTEX 	= bibtex
DVIPS 	= dvips
DVIPDF 	= dvipdft
XDVI	= xdvi -gamma 4
GH 		= gv

#Used to specify the correct build path for tex on os2 server
BUILDDIR = /usr/local/apps/tex_live/current/bin/x86_64-linux

EXAMPLES	= $(wildcard *.c)
SRC	  	:= $(shell egrep -l '^[^%]*\\begin\{document\}' *.tex)
TRG 		= $(SRC:%.tex=%.dvi)
PSF 		= $(SRC:%.tex=%.ps)
PDF 		= $(SRC:%.tex=%.pdf)

#all: pdf ps $(TARGETS)
all:
	make ps; make pdf;

pdf: $(PDF)

ps: $(PSF)

$(TRG): %.dvi: %.tex $(EXAMPLES)
	#use pygments to include source code
	#pygmentize -f latex -o __${EXAMPLES}.tex ${EXAMPLES}

	$(BUILDDIR)/$(LATEX) $<
	$(BUILDDIR)/$(BIBTEX) $(<:%.tex=%)
	$(BUILDDIR)/$(LATEX) $<
	$(BUILDDIR)/$(LATEX) $<
	#rm __${SRC}.tex
	#remove pygmentized output to avoid cluttering up directory

$(PSF):%.ps: %.dvi
	$(BUILDDIR)/$(DVIPS) -R -Poutline -t letter $< -o $@

$(PDF): %.pdf: %.ps
	ps2pdf $<

show: $(TRG)
	@for i in $(TRG) ; do $(XDVI) $$i & done

showps: $(PSF)
	@for i in $(PSF) ; do $(GH) $$i & done

clean:
	rm -f *.pdf *.ps *.dvi *.out *.log *.aux *.bbl *.blg *.pyg *.synctex.gz

.PHONY: all show clean ps pdf showps

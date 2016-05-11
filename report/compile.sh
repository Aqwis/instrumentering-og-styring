#!/bin/bash
pdflatex top.tex
bibtex top
pdflatex top.tex
pdflatex top.tex

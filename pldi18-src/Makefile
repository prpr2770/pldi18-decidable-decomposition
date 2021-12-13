all:
	pdflatex main.tex
	bibtex main.aux || true
	pdflatex main.tex
	pdflatex main.tex

trim:
	pdfcrop modules-fig.pdf modules-fig-trim.pdf

README.html: README.md github-pandoc.css
	pandoc README.md -s -c github-pandoc.css --self-contained --toc -o README.html

# makefile for latex document
LATEX=$(VOUCHBIN)latex
DVIPS=$(VOUCHBIN)dvips
document:	$(DOC).ps

$(DOC).dvi:	$(DOC).tex 
		$(LATEX) $(DOC)

$(DOC).ps:	$(DOC).dvi
	$(DVIPS) -t landscape $(DOC)

ps:	$(DOC).ps

dvi:	$(DOC).dvi

printdoc:	$(DOC).ps
	$(PSPRINT) $(DOC).ps
	
print:	printdoc


test:	document
	dviselect 1 $(DOC).dvi tmp.dvi
	(dvips tmp; psprint tmp.ps)

#one possible test under exptools, just printing one page
exp-test: $(DOC).dvi
	$(DVIPS) -o tmp.ps -t landscape -n 1 $(DOC)
	$(PSPRINT) tmp.ps


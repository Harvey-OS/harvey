!include "..\mkconfig.nmk"
!include "..\$(SYSHOST)\mkhost"
!include "..\$(OBJDIR)\makefile"

LIB=libimage.$A
OFILES_C= \
	alloc.$O\
	arith.$O\
	bezier.$O\
	buildfont.$O\
	bytesperline.$O\
	creadimage.$O\
	cursor.$O\
	defont.$O\
	draw.$O\
	ellipse.$O\
	font.$O\
	freesubfont.$O\
	getsubfont.$O\
	init.$O\
	line.$O\
	loadimage.$O\
	mkfont.$O\
	openfont.$O\
	poly.$O\
	readimage.$O\
	readsubfont.$O\
	rectclip.$O\
	replclipr.$O\
	rgb.$O\
	string.$O\
	stringsubfont.$O\
	stringwidth.$O\
	subfont.$O\
	subfontcache.$O\
	subfontname.$O\
	unloadimage.$O\
	window.$O\
	writeimage.$O\

HFILES=\
	../include/memimage.h

!include <..\$(SYSTARG)\mksyslib.nmk>

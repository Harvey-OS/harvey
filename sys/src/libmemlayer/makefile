!include ..\mkconfig.nmk
!include ..\$(SYSHOST)\mkhost
!include ..\$(OBJDIR)\makefile

CC_LOCAL=  

dotodir=.

LIB=libmemlayer.$A
OFILES_C=\
	$(dotodir)\draw.$O\
	$(dotodir)\lalloc-$(SYSTARG).$O\
	$(dotodir)\layerop.$O\
	$(dotodir)\ldelete.$O\
	$(dotodir)\lhide.$O\
	$(dotodir)\line.$O\
	$(dotodir)\load.$O\
	$(dotodir)\lorigin.$O\
	$(dotodir)\lsetrefresh.$O\
	$(dotodir)\ltofront.$O\
	$(dotodir)\ltorear.$O

HFILES=\
	..\include\image.h\
	..\include\memimage.h\
	..\include\memlayer.h

!include ..\$(SYSTARG)\mksyslib.nmk

$(dotodir)\draw.$O:		draw.c
$(dotodir)\lalloc.$O:		lalloc.c
$(dotodir)\layerop.$O:		layerop.c
$(dotodir)\ldelete.$O:		ldelete.c
$(dotodir)\lhide.$O:		lhide.c
$(dotodir)\line.$O:		line.c
$(dotodir)\lsetrefresh.$O:	lsetrefresh.c
$(dotodir)\ltofront.$O:		ltofront.c
$(dotodir)\ltorear.$O:		ltorear.c

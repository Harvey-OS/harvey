# Contributed drivers not found in the current distribution
# We reinsert them whenever we download a new distribution.

### ------------------------- Plan 9 bitmaps -------------------------- ###
### Note: this driver was contributed by a user: please contact          ###
###       Russ Cox <rsc@plan9.bell-labs.com> if you have questions.      ###

plan9_=$(GLOBJ)gdevplan9.$(OBJ)
$(DD)plan9.dev : $(plan9_) $(DD)page.dev
	$(SETPDEV) $(DD)plan9 $(plan9_)

$(GLOBJ)gdevplan9.$(OBJ) : $(GLSRC)gdevplan9.c $(PDEVH) $(gsparam_h) $(gxlum_h)
	$(GLCC) $(GLO_)gdevplan9.$(OBJ) $(C_) $(GLSRC)gdevplan9.c

### -------------- cdj850 - HP 850c Driver under development ------------- ###
### For questions about this driver, please contact:                       ###
###       Uli Wortmann (uliw@erdw.ethz.ch)                                 ###

cdeskjet8_=$(GLOBJ)gdevcd8.$(OBJ) $(HPPCL)

$(DD)cdj850.dev : $(cdeskjet8_) $(DD)page.dev
	$(SETPDEV2) $(DD)cdj850 $(cdeskjet8_)

$(DD)cdj670.dev : $(cdeskjet8_) $(DD)page.dev
	$(SETPDEV2) $(DD)cdj670 $(cdeskjet8_)

$(DD)cdj890.dev : $(cdeskjet8_) $(DD)page.dev
	$(SETPDEV2) $(DD)cdj890 $(cdeskjet8_)

$(DD)cdj1600.dev : $(cdeskjet8_) $(DD)page.dev
	$(SETPDEV2) $(DD)cdj1600 $(cdeskjet8_)

$(GLOBJ)gdevcd8.$(OBJ) : $(GLSRC)gdevcd8.c $(PDEVH) $(math__h)\
 $(gsparam_h) $(gxlum_h) $(gdevpcl_h)
	$(GLCC) $(GLO_)gdevcd8.$(OBJ) $(C_) $(GLSRC)gdevcd8.c


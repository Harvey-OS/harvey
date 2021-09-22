void error AA((void));
void jumpout AA((void));
void parsearguments AA((void));
void initialize AA((void));
void openinput AA((void));
boolean zinputln AA((textfile f));
#define inputln(f) zinputln((textfile) (f))
boolean linesdontmatch AA((void));
void primethechangebuffer AA((void));
void checkchange AA((void));
void getline AA((void));
namepointer lookup AA((void));
void getnext AA((void));
void zflushbuffer AA((eightbits b,boolean percent));
#define flushbuffer(b, percent) zflushbuffer((eightbits) (b), (boolean) (percent))
void breakout AA((void));
void zoutstr AA((namepointer p));
#define outstr(p) zoutstr((namepointer) (p))
void zoutname AA((namepointer p));
#define outname(p) zoutname((namepointer) (p))
void zoutmacandname AA((ASCIIcode n,namepointer p));
#define outmacandname(n, p) zoutmacandname((ASCIIcode) (n), (namepointer) (p))
void zcopy AA((integer firstloc));
#define copy(firstloc) zcopy((integer) (firstloc))
void dothetranslation AA((void));

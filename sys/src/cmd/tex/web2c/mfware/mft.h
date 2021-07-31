void error();
void jumpout();
void scanargs();
void initialize();
void openinput();
boolean zinputln();
#define inputln(f) zinputln((textfile) (f))
boolean linesdontmatch();
void primethechangebuffer();
void checkchange();
void getline();
namepointer lookup();
void getnext();
void zflushbuffer();
#define flushbuffer(b, percent) zflushbuffer((eightbits) (b), (boolean) (percent))
void breakout();
void zoutstr();
#define outstr(p) zoutstr((namepointer) (p))
void zoutname();
#define outname(p) zoutname((namepointer) (p))
void zoutmacandname();
#define outmacandname(n, p) zoutmacandname((ASCIIcode) (n), (namepointer) (p))
void zcopy();
#define copy(firstloc) zcopy((integer) (firstloc))
void dothetranslation();

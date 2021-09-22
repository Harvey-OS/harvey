void parsearguments AA((void));
void initialize AA((void));
void openmpxfile AA((void));
void opendvifile AA((void));
boolean opentfmfile AA((void));
boolean openvffile AA((void));
void readtfmword AA((void));
integer getbyte AA((void));
integer signedbyte AA((void));
integer gettwobytes AA((void));
integer signedpair AA((void));
integer getthreebytes AA((void));
integer signedtrio AA((void));
integer signedquad AA((void));
void zprintchar AA((eightbits c));
#define printchar(c) zprintchar((eightbits) (c))
void zendcharstring AA((integer l));
#define endcharstring(l) zendcharstring((integer) (l))
void zprintfont AA((integer f));
#define printfont(f) zprintfont((integer) (f))
void zerrprintfont AA((integer f));
#define errprintfont(f) zerrprintfont((integer) (f))
integer zmatchfont AA((integer ff,boolean exact));
#define matchfont(ff, exact) zmatchfont((integer) (ff), (boolean) (exact))
void zdefinefont AA((integer e));
#define definefont(e) zdefinefont((integer) (e))
void zinTFM AA((integer f));
#define inTFM(f) zinTFM((integer) (f))
integer zfirstpar AA((eightbits o));
#define firstpar(o) zfirstpar((eightbits) (o))
void zinVF AA((integer f));
#define inVF(f) zinVF((integer) (f))
integer zselectfont AA((integer e));
#define selectfont(e) zselectfont((integer) (e))
void finishlastchar AA((void));
void zdosetchar AA((integer f,integer c));
#define dosetchar(f, c) zdosetchar((integer) (f), (integer) (c))
void zdosetrule AA((integer ht,integer wd));
#define dosetrule(ht, wd) zdosetrule((integer) (ht), (integer) (wd))
void startpicture AA((void));
void stoppicture AA((void));
void dopush AA((void));
void dopop AA((void));
void zsetvirtualchar AA((integer f,integer c));
#define setvirtualchar(f, c) zsetvirtualchar((integer) (f), (integer) (c))
void dodvicommands AA((void));

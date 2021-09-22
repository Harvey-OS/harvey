void zprintpacket AA((pcktpointer p));
#define printpacket(p) zprintpacket((pcktpointer) (p))
void zprintfont AA((fontnumber f));
#define printfont(f) zprintfont((fontnumber) (f))
void printoptions AA((void));
void jumpout AA((void));
void zconfusion AA((pcktpointer p));
#define confusion(p) zconfusion((pcktpointer) (p))
void zoverflow AA((pcktpointer p,int16u n));
#define overflow(p, n) zoverflow((pcktpointer) (p), (int16u) (n))
void badtfm AA((void));
void badfont AA((void));
void baddvi AA((void));
void parsearguments AA((void));
void initialize AA((void));
pcktpointer makepacket AA((void));
pcktpointer newpacket AA((void));
void flushpacket AA((void));
int8 pcktsbyte AA((void));
int8u pcktubyte AA((void));
int16 pcktspair AA((void));
int16u pcktupair AA((void));
int24 pcktstrio AA((void));
int24u pcktutrio AA((void));
integer pcktsquad AA((void));
void zpcktfour AA((integer x));
#define pcktfour(x) zpcktfour((integer) (x))
void zpcktchar AA((boolean upd,integer ext,eightbits res));
#define pcktchar(upd, ext, res) zpcktchar((boolean) (upd), (integer) (ext), (eightbits) (res))
void zpcktunsigned AA((eightbits o,integer x));
#define pcktunsigned(o, x) zpcktunsigned((eightbits) (o), (integer) (x))
void zpcktsigned AA((eightbits o,integer x));
#define pcktsigned(o, x) zpcktsigned((eightbits) (o), (integer) (x))
void zmakename AA((pcktpointer e));
#define makename(e) zmakename((pcktpointer) (e))
widthpointer zmakewidth AA((integer w));
#define makewidth(w) zmakewidth((integer) (w))
boolean findpacket AA((void));
void zstartpacket AA((typeflag t));
#define startpacket(t) zstartpacket((typeflag) (t))
void buildpacket AA((void));
void readtfmword AA((void));
void zcheckchecksum AA((integer c,boolean u));
#define checkchecksum(c, u) zcheckchecksum((integer) (c), (boolean) (u))
void zcheckdesignsize AA((integer d));
#define checkdesignsize(d) zcheckdesignsize((integer) (d))
widthpointer zcheckwidth AA((integer w));
#define checkwidth(w) zcheckwidth((integer) (w))
void loadfont AA((void));
fontnumber zdefinefont AA((boolean load));
#define definefont(load) zdefinefont((boolean) (load))
integer dvilength AA((void));
void zdvimove AA((integer n));
#define dvimove(n) zdvimove((integer) (n))
int8 dvisbyte AA((void));
int8u dviubyte AA((void));
int16 dvispair AA((void));
int16u dviupair AA((void));
int24 dvistrio AA((void));
int24u dviutrio AA((void));
integer dvisquad AA((void));
int31 dviuquad AA((void));
int31 dvipquad AA((void));
integer dvipointer AA((void));
void dvifirstpar AA((void));
void dvifont AA((void));
void zdvidofont AA((boolean second));
#define dvidofont(second) zdvidofont((boolean) (second))
int8u vfubyte AA((void));
int16u vfupair AA((void));
int24 vfstrio AA((void));
int24u vfutrio AA((void));
integer vfsquad AA((void));
integer vffix1 AA((void));
integer vffix2 AA((void));
integer vffix3 AA((void));
integer vffix3u AA((void));
integer vffix4 AA((void));
int31 vfuquad AA((void));
int31 vfpquad AA((void));
int31 vffixp AA((void));
void vffirstpar AA((void));
void vffont AA((void));
void vfdofont AA((void));
boolean dovf AA((void));
void inputln AA((void));
boolean zscankeyword AA((pcktpointer p,int7 l));
#define scankeyword(p, l) zscankeyword((pcktpointer) (p), (int7) (l))
integer scanint AA((void));
void scancount AA((void));
void dialog AA((void));
void zoutpacket AA((pcktpointer p));
#define outpacket(p) zoutpacket((pcktpointer) (p))
void zoutfour AA((integer x));
#define outfour(x) zoutfour((integer) (x))
void zoutchar AA((boolean upd,integer ext,eightbits res));
#define outchar(upd, ext, res) zoutchar((boolean) (upd), (integer) (ext), (eightbits) (res))
void zoutunsigned AA((eightbits o,integer x));
#define outunsigned(o, x) zoutunsigned((eightbits) (o), (integer) (x))
void zoutsigned AA((eightbits o,integer x));
#define outsigned(o, x) zoutsigned((eightbits) (o), (integer) (x))
void zoutfntdef AA((fontnumber f));
#define outfntdef(f) zoutfntdef((fontnumber) (f))
boolean startmatch AA((void));
void dopre AA((void));
void dobop AA((void));
void doeop AA((void));
void dopush AA((void));
void dopop AA((void));
void doxxx AA((void));
void doright AA((void));
void dodown AA((void));
void dowidth AA((void));
void dorule AA((void));
void dochar AA((void));
void dofont AA((void));
void pcktfirstpar AA((void));
void dovfpacket AA((void));
void dodvi AA((void));
void closefilesandterminate AA((void));

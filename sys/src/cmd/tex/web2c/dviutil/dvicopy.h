void zprintpacket();
#define printpacket(p) zprintpacket((pcktpointer) (p))
void zprintfont();
#define printfont(f) zprintfont((fontnumber) (f))
void jumpout();
void zconfusion();
#define confusion(p) zconfusion((pcktpointer) (p))
void zoverflow();
#define overflow(p, n) zoverflow((pcktpointer) (p), (int16u) (n))
void badtfm();
void badfont();
void baddvi();
void initialize();
pcktpointer makepacket();
pcktpointer newpacket();
void flushpacket();
int8 pcktsbyte();
int8u pcktubyte();
int16 pcktspair();
int16u pcktupair();
int24 pcktstrio();
int24u pcktutrio();
integer pcktsquad();
void zpcktfour();
#define pcktfour(x) zpcktfour((integer) (x))
void zpcktchar();
#define pcktchar(upd, ext, res) zpcktchar((boolean) (upd), (integer) (ext), (eightbits) (res))
void zpcktunsigned();
#define pcktunsigned(o, x) zpcktunsigned((eightbits) (o), (integer) (x))
void zpcktsigned();
#define pcktsigned(o, x) zpcktsigned((eightbits) (o), (integer) (x))
void zmakename();
#define makename(e) zmakename((pcktpointer) (e))
widthpointer zmakewidth();
#define makewidth(w) zmakewidth((integer) (w))
boolean findpacket();
void zstartpacket();
#define startpacket(t) zstartpacket((typeflag) (t))
void buildpacket();
void readtfmword();
void zcheckchecksum();
#define checkchecksum(c, u) zcheckchecksum((integer) (c), (boolean) (u))
void zcheckdesignsize();
#define checkdesignsize(d) zcheckdesignsize((integer) (d))
widthpointer zcheckwidth();
#define checkwidth(w) zcheckwidth((integer) (w))
fontnumber makefont();
integer dvilength();
void zdvimove();
#define dvimove(n) zdvimove((integer) (n))
int8 dvisbyte();
int8u dviubyte();
int16 dvispair();
int16u dviupair();
int24 dvistrio();
int24u dviutrio();
integer dvisquad();
int31 dviuquad();
int31 dvipquad();
integer dvipointer();
void dvifirstpar();
void dvifont();
void zdvidofont();
#define dvidofont(second) zdvidofont((boolean) (second))
int8u vfubyte();
int16u vfupair();
int24 vfstrio();
int24u vfutrio();
integer vfsquad();
integer vffix1();
integer vffix2();
integer vffix3();
integer vffix3u();
integer vffix4();
int31 vfuquad();
int31 vfpquad();
int31 vffixp();
void vffirstpar();
void vffont();
void vfdofont();
boolean dovf();
void inputln();
boolean zscankeyword();
#define scankeyword(p, l) zscankeyword((pcktpointer) (p), (int7) (l))
integer scanint();
void scancount();
void dialog();
void zoutpacket();
#define outpacket(p) zoutpacket((pcktpointer) (p))
void zoutfour();
#define outfour(x) zoutfour((integer) (x))
void zoutchar();
#define outchar(upd, ext, res) zoutchar((boolean) (upd), (integer) (ext), (eightbits) (res))
void zoutunsigned();
#define outunsigned(o, x) zoutunsigned((eightbits) (o), (integer) (x))
void zoutsigned();
#define outsigned(o, x) zoutsigned((eightbits) (o), (integer) (x))
void zoutfntdef();
#define outfntdef(f) zoutfntdef((fontnumber) (f))
boolean startmatch();
void dopre();
void dobop();
void doeop();
void dopush();
void dopop();
void doxxx();
void doright();
void dodown();
void dowidth();
void dorule();
void dochar();
void dofont();
void pcktfirstpar();
void dovfpacket();
void dodvi();
void closefilesandterminate();

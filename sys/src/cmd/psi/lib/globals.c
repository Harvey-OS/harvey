#include <u.h>
#include <libc.h>
#include "system.h"
#ifdef Plan9
#include <libg.h>
#endif
# include "stdio.h"
# include "defines.h"
# include "object.h"
# include "path.h"
#include "njerq.h"
# include "graphics.h"
#include "cache.h"

char		*Prog_name ;
int		Line_no ;
long	Char_ct;

struct graphics	Graphics ;
struct	object	Device ;
struct object FontMatrix, FID, ImageMatrix, FontBBox, none, FontDirectory,
	FontType, Encoding, Buildchar, BHeight, Chardata, CharStrings,FontInfo,
	nullstr, true, false, zero, null, lbrak, rbrak, notdef, Ascent, unknown, blank,
	lbrace, rbrace;
struct object filename;
struct object Charstrings, SCharstrings, Fontinfo, standardencoding;
struct object xalpha[52], lalpha[52], saveobj;
struct pspoint zeropt;

struct cachefont cachefont[CACHE_MLIMIT];
struct cachefont *currentcache;
double savex, savey;

struct	dict	*Systemdict ;
struct 	dict	*statusdict ;
int minx= XMAX, maxx=0, miny=YMAX, maxy=0;
char pagestr[10];
int pageno, prolog=0, page, pageflag, prologline, skipflag, restart, reversed=0;
int saveflag;
int searchflg=0;
long prologend;
int nofontinit = 0;
struct Point anchor, corn, orig, p_anchor;
FILE *xerr, *fpcache;
int Fullbits=0;
int dontcache=0, dontask=0, bbflg = 0;
int stdflag=0;
int current_type;
int cacheit;
Rectangle cborder;
int msize = 0;
int cacheout=0;
int charpathflg=0;
int Fonts = 0;
int blimit = CACHE_BLIMIT,bmax=0,mmax=CACHE_MLIMIT,cmax=CACHE_CLIMIT,csize = 0,bsize=0;
double yadj, nxadj;
int fontchanged;
int currentx, currenty;
int chct, shown;
int texcode;		/*kluge for tex lies & tall space*/

FILE *currentfile;
int decryptf=0, d_unget;
int mdebug=0;
int	errno ;
int instring=0;
int merganthal = 0;
int korean = 0;
int microsoft = 0;

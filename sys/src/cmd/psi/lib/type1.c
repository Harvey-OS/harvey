/*
 * Type 1 font routines. Perhaps 90% complete (including hints) and 50% tested.
 * What's missing does not appear to be important for most fonts. The tuning
 * process consists of locking key points of the outline to pixels.
 * Notably missing is all special code to handle PaintType 2
 * fonts. Other things are undoubtedly not quite right, and finding mistakes
 * is not trivial, particularly when psi itself is also often a suspect. The
 * fill routine has been hacked, but it was just a quick job. Things are still
 * not correct in do_fill()!
 *
 */

# include <u.h>
#include <libc.h>
# include "system.h"
#ifdef Plan9
#include <libg.h>
#endif

# include "stdio.h"
# include "ctype.h"
# include "defines.h"
# include "object.h"
# include "dict.h"
# include "path.h"
#include "njerq.h"
# include "graphics.h"
#include "comm.h"
#include "cache.h"
# include "omagic.h"


static int xflg=0;
/*
 *
 * Red book documented entries in Adobe fonts. Included all of them, even though
 * some aren't used.
 *
 */
struct object	n_FontName;
static struct object	n_PaintType;
static struct object	n_Metrics;
static struct object	n_StrokeWidth;
static struct object	n_UniqueID;
static struct object	n_Private;

/*
 *
 * Entries in a font's Private dictionary.
 *
 */

static struct object	n_password;
static struct object	n_Subrs;
static struct object	n_OtherSubrs;
static struct object	n_BlueValues;
static struct object	n_BlueFuzz;
static struct object	n_BlueShift;
static struct object	n_BlueScale;
static struct object	n_FudgeBands;

static struct object	n_descend;
static struct object	n_ascend;
static struct object	n_overshoot;
static struct object	n_xover;
static struct object	n_capover;
static struct object	n_aover;
static struct object	n_halfsw;
static struct object	n_strokewidth;
static struct object	n_baseline;
static struct object	n_capheight;
static struct object	n_bover;
static struct object	n_xheight;

/*
 *
 * internaldict is a hidden dictionary found in all Adobe interpreters. Includes,
 * among other things, names and special operators used with type 1 fonts.
 *
 */

static struct object	internaldict;

static struct object	n_strtlck;
static struct object	n_lck;
static struct object	n_xlck;
static struct object	n_ylck;
static struct object	n_superexec;
static struct object	n_CCRun;
static struct object	n_FlxProc;
static struct object	n_locktype;
static struct object	n_erosion;
static struct object	n_sbx;
static struct object	n_sby;

/*
 *
 * A character description is an encrypted string that's passed as an argument
 * to ExecCharString(). First four bytes and SHOWSTART (file magic.h) initialize
 * Key. After that clear text is recovered using,
 *
 *		clear = ((Key >> 8) ^ *CharString) & 0xFF;
 *		Key = (Key + *CharString++) * MAGIC1 + MAGIC2;
 *
 * Same algorithm is used in eexec. Only significant difference is the starting
 * point.
 *
 */

static unsigned char	*CharString;
static unsigned int	Key;

/*
 *
 * Globals - from the font dictionary.
 *
 */

static struct object	private;
static struct dict *Private;
static int	PaintType;
static double	StrokeWidth;
static struct object	tEncoding;
static struct object	Metrics;

/*
 *
 * Metric info for an individual chararacter. Pulled out of the font's Metrics
 * dictionary (if it exists) using Encoding and the character's code.
 *
 */

static struct object	Metric;
static int	nMetric;

/*
 *
 * Lock tables do what you might think, namely lock key points in user (actually
 * font) space to device space pixels. XlckTable and YlckTable are used in exactly
 * the same way, but are built differently. Tables include vector displacements
 * (tied to a font space coordinate) along each axis that move that coordinate
 * onto a device space pixel. If a lock table includes an entry for more than
 * one point, a scale factor is used to smoothly adjust the shift between the
 * two lock points.
 *
 * Lock tables are built from hints encoded in a CharString. Locking hints pass
 * a pair of numbers to routines that build the lock tables, and those routines
 * calculate the vector adjustment needed (in font space) so the middle of the
 * interval lands on a pixel. Execptions occur only when building the y lock
 * table and only when one or the other end point lands in a BlueValue interval.
 *
 */

static int	Locktype;
static int	IsLocked;
static LckTable	XlckTable[LCKTBLSZ+1];
static LckTable	YlckTable[LCKTBLSZ+1];

/*
 *
 * BlueValues is an array of at most 6 pairs of numbers and is initialized from
 * the BlueValues entry in a font's Private dictionary. nBlueValues is the number
 * entries in the array. BlueValue pairs mark important intervals along the y
 * axes e.g. the baseline, top of smalls, and top of caps. When a lock pair lands
 * in one of these intervals the vector entry in YlckTable[] is adjusted so that
 * endpoint (rather than the center) lands on a pixel.
 *
 */

static double	BlueValues[12];
static int	nBlueValues;

/*
 *
 * Three additional numbers are also used to build YlckTable. They're assigned
 * defaults (file magic.h) if there are no corresponding entries in the Private
 * dictionary.
 *
 */

static double	BlueShift;
static double	BlueScale;
static double	BlueFuzz;

/*
 *
 * Special internaldict entries - for debugging psi. As an example,
 *
 *	1183615869 internaldict begin
 *		/debug 1 def
 *	end
 *
 *
 */
static struct pspoint	LastPoint;
static int	IsRelative;

static struct object	n_debug;
static struct object	n_unlock;
static struct object	n_nohints;
static struct object	n_grabpath;
static struct object	n_flatness;
static struct object	n_miterlimit;
static struct object	n_linewidth;

static int	tdebug;
static int	unlock;
static int	nohints;
static int	grabpath;

/*
 *
 * Need to access a few external variables. For convenience use a systemdict
 * struct object rather than psi's Systemdict dict pointer.
 *
 */

static struct object	systemdict;
struct dict	*Internaldict;
struct object t1dict;
struct dict *T1dict;
struct object T1Names;
struct object T1Dir;
int T1Dirleng;
static struct pspoint		width;			/* character advance */
static struct pspoint		cp;			/* currentpoint */
static struct pspoint		sb;			/* and side bearings */
static struct pspoint	xleft;
unsigned int d_key;
int d_unget=EOF;
static int inseac;
static double orx;

void
type1init(void)
{

/*
 *
 * Builds name objects for internaldict and type1 fonts, creates the internaldict
 * dictionary, and installs special internaldict operators.
 *
 */
	int i;

	n_PaintType = cname("PaintType");
	n_Metrics = cname("Metrics");
	n_StrokeWidth = cname("StrokeWidth");
	n_UniqueID = cname("UniqueID");
	n_Private = cname("Private");

	n_password = cname("password");
	n_Subrs = cname("Subrs");
	n_OtherSubrs = cname("OtherSubrs");
	n_BlueValues = cname("BlueValues");
	n_BlueFuzz = cname("BlueFuzz");
	n_BlueShift = cname("BlueShift");
	n_BlueScale = cname("BlueScale");
	n_FudgeBands = cname("FudgeBands");
	n_descend = cname("descend");
	n_ascend = cname("ascend");
	n_overshoot = cname("overshoot");
	n_xover = cname("xover");
	n_capover = cname("capover");
	n_aover = cname("aover");
	n_halfsw = cname("halfsw");
	n_strokewidth = cname("strokewidth");
	n_baseline = cname("baseline");
	n_capheight = cname("capheight");
	n_bover = cname("bover");
	n_xheight = cname("xheight");

	n_strtlck = cname("strtlck");
	n_lck = cname("lck");
	n_xlck = cname("xlck");
	n_ylck = cname("ylck");
	n_superexec = cname("superexec");
	n_CCRun = cname("CCRun");
	n_FlxProc = cname("FlxProc");
	n_locktype = cname("locktype");
	n_erosion = cname("erosion");
	n_sbx = cname("sbx");
	n_sby = cname("sby");

	n_debug = cname("debug");
	n_unlock = cname("unlock");
	n_nohints = cname("nohints");
	n_grabpath = cname("grabpath");
	n_flatness = cname("flatness");
	n_miterlimit = cname("miterlimit");
	n_linewidth = cname("linewidth");

	internaldict = makedict(50);
	Internaldict=internaldict.value.v_dict;
	dictput(Internaldict, n_strtlck, makeoperator(strtlckOP));
	dictput(Internaldict, n_lck, makeoperator(lckOP));
	dictput(Internaldict, n_xlck, makeoperator(xlckOP));
	dictput(Internaldict, n_ylck, makeoperator(ylckOP));
	dictput(Internaldict, n_superexec, makeoperator(superexecOP));
	dictput(Internaldict, n_CCRun, makeoperator(CCRunOP));

	systemdict.type = OB_DICTIONARY;	/* psi doesn't have one */
	systemdict.xattr = XA_LITERAL;
	systemdict.value.v_dict = Systemdict;
}

void
eexecOP(void)

{
	int	i, ch;
	struct object	obj, obj1;
	FILE *fp;

/*
 *
 * Executes an encrypted file or string. First four bytes, after leading white
 * space, are the key. Reads hex if the first character of the key is hex. The
 * decryption algorithm (implemented in f_getc()) does,
 *
 *		cypher = getchar();
 *		clear = ((key >> 8) ^ cypher) & 0xFF;
 *		key = (key + cypher) * MAGIC1 + MAGIC2;
 *		return(clear);
 *
 * Probably should begin systemdict and end it in f_close().
 *
 */
	obj = pop();
	if ( obj.type == OB_FILE ) {
		if ( obj.value.v_file.access == AC_NONE )
			pserror("invalidaccess", "eexec");
		fp = obj.value.v_file.fp;
		obj1 = obj;
	}
	else if(obj.type == OB_STRING){
		fp = sopen(obj.value.v_string);
		obj1 = makefile(fp, AC_EXECUTEONLY, XA_EXECUTABLE);
	}
	else pserror("typecheck", "eexec");
	while ( (ch = f_getc(fp)) != EOF && isspace(ch) );
	if ( isascii(ch) && isxdigit(ch) && obj.type == OB_FILE ) {
			/* all hex if first char is except for strings?*/
		un_getc(ch, fp);
		ch = fgetx(fp);
		ch = (ch << 4) | fgetx(fp);
		decryptf = 1;
	}
	else decryptf = 2;
	d_key = (EEXECSTART + ch) * MAGIC1 + MAGIC2;
	for ( i = 0; i < 3; i++ ){	/* finish with the key */
		ch = df_getc(fp);
	}
	execpush(obj1);
}

void
internaldictOP(void)	/* If the password's right leave internaldict on the operand stack.*/
{
	struct object	password;

	password = pop();
	if ( password.value.v_int != INTERNALPASSWORD )
		pserror("invalidaccess", "internaldict");
	push(internaldict);
}

void
strtlckOP(void)	/* Clear both lock tables.*/
{
	XlckTable[0].count = YlckTable[0].count = 0;
}

void
xlckOP(void)	/* Top two numbers on the stack go in XlckTable. May have the order wrong.*/
{

	double	tail, tip;

	tail = PopDouble();
	tip = PopDouble();
	InsertLck(tail, tip, XlckTable);
}

void
ylckOP(void)	/* Top two numbers go in YlckTable. Order may again be wrong.*/
{
	double	tail, tip;

	tail = PopDouble();
	tip = PopDouble();
	InsertLck(tail, tip, YlckTable);
}

void
lckOP(void)
{
	double	x, y;
	struct pspoint	p;

	p.y = PopDouble();
	p.x = PopDouble();
	p = Lck(p);
	push(makereal(p.x));
	push(makereal(p.y));
}

void
superexecOP(void)	/* Execute an object with all access checking disabled.*/
{
	execpush(pop());
	execute();
}

void
CCRunOP(void)
{}
void
ExecCharString(struct dict *font, int code, int charpath, struct object bool)
{
	double		xyvec[24], asb, xx, yy;		/* internal stack */
	register	top = 0;
	struct source	source[10];		/* for Subr returns */
	int		nsource = 0;
	struct pspoint	p, p0, p1, p2, w;
	Vector		vec;
	struct object	obj, fontobj, name;
	int		issmall, i, n, c, d, achar, bchar;
	static int didname;

	if ( tdebug = IntLookup(Internaldict, n_debug, 0) )
		fprintf(stderr, "code=%c\n", code);
	fontobj.type = OB_DICTIONARY;
	fontobj.xattr = XA_LITERAL;
	fontobj.value.v_dict = font;
	Key = SHOWSTART;

	top = 0;
	IsRelative = 0;
	LastPoint.x = LastPoint.y = 0.0;
	XlckTable[0].count = YlckTable[0].count = 0;

	GetPrivate(font);


	if(merganthal && !inseac){
		if(merganthal&&!didname){
			name = dictget(font, n_FontName);
			didname++;
			fprintf(stderr,"fontname ");
			push(name);
			printOP();
		}
		if(code == 040)fprintf(stderr,"\ncode sp %o %d\n", code, code);
		else if(code == 011)fprintf(stderr,"\ncode ht %o %d\n", code, code);
		else if(code == 012)fprintf(stderr,"\ncode nl %o %d\n", code, code);
		else if(code >=0200 || code < 040)fprintf(stderr,"\ncode x %o %d\n",code,code);
		else fprintf(stderr,"\ncode %c %o %d\n",code,code, code);
	}

	if ( (PaintType = IntLookup(font, n_PaintType, -1)) < 0 || PaintType > 3 )
		pserror("invalidfont", "bad PaintType");

	if ( (StrokeWidth = NumLookup(font, n_StrokeWidth, 0.0)) < 0 )
		pserror("invalidfont", "bad StrokeWidth");
	if(StrokeWidth == 0.)StrokeWidth = 1.;
	nMetric = GetMetricInfo(font, code);
	if(nMetric == -2)return;

	issmall = SetLocktype();
	Graphics.incharpath = 1;
	Graphics.origin.x = Graphics.CTM[4];
	Graphics.origin.y = Graphics.CTM[5];
	Graphics.CTM[4] = 0.0;
	Graphics.CTM[5] = 0.0;

	Graphics.flat = FLATNESS;
	Graphics.miterlimit = (PaintType == 2) ? TYPE2MITER : DEFAULTMITER;
	/* Set other things like the dash pattern and linewidth? */

	if(!inseac)newpathOP();
	if(inseac != 2){
		orx = 0.;
		Moveto(cp);
	}

	if ( tdebug ) {
		unlock = IntLookup(Internaldict, n_unlock, 0);
		grabpath = IntLookup(Internaldict, n_grabpath, 0);
		BlueFuzz = NumLookup(Internaldict, n_BlueFuzz, BlueFuzz);
		BlueShift = NumLookup(Internaldict, n_BlueShift, BlueShift);
		BlueScale = NumLookup(Internaldict, n_BlueScale, BlueScale);
		StrokeWidth = NumLookup(Internaldict, n_StrokeWidth, StrokeWidth);
		Graphics.flat = NumLookup(Internaldict, n_flatness, Graphics.flat);
		Graphics.miterlimit = NumLookup(Internaldict, n_miterlimit, Graphics.miterlimit);
		if ( nohints ) unlock = 1;
	} else unlock = nohints = grabpath = 0;
unlock=1;
	dictput(Internaldict, n_locktype, makeint(Locktype));
	dictput(Internaldict, n_sbx, makereal(sb.x));
	dictput(Internaldict, n_sby, makereal(sb.y));
	for ( i = 0; i < 4; i++ )
		Newchar();
	c = Newchar();
	while ( 1 ) {
		if ( c >= 0x20 && c <= 0xF6 ){
			xyvec[top++] = c - 0x8B;
/*fprintf(stderr,"top1 %d val %f\n",top, xyvec[top-1]);*/
		}
		else if ( c >= 0xF7 && c <= 0xFA ){
			xyvec[top++] = 0x6C + (((c - 0xF7)<<8) | Newchar());
/*fprintf(stderr,"top2 %d val %f\n",top, xyvec[top-1]);*/
		}
		else if ( c >= 0xFB && c <= 0xFE ){
			xyvec[top++] = -(0x6C + (((c - 0xFB)<<8) | Newchar()));
/*fprintf(stderr,"top3 %d val %f\n",top, xyvec[top-1]);*/

		}
		else if ( c == 0xFF ) {
			for ( i = 0, c = 0; i < 4; i++ )
				c = (c<<8) | Newchar();
			xyvec[top++] = c;
/*fprintf(stderr,"top4 %d val %f\n",top, xyvec[top-1]);*/
		} else {
			if ( tdebug )
				fprintf(stderr, " case %x top %d\n", c, top);
			switch ( c ) {
			    case HSTEM:
				if ( Locktype ) {
					xyvec[0] += sb.y;
					xyvec[1] += xyvec[0];
					vec = GetYlckVector(xyvec[0], xyvec[1]);
					InsertLck(vec.tail, vec.tip, YlckTable);
				}
				break;

			    case VSTEM:
				if ( Locktype ) {
					xyvec[0] += sb.x + orx;
					xyvec[1] += xyvec[0];
					vec = GetXlckVector(xyvec[0], xyvec[1]);
					InsertLck(vec.tail, vec.tip, XlckTable);
				}
				break;

			    case VMOVETO:
				cp.y += xyvec[0];
				if ( Locktype )
					p = LockPoint(cp);
				else p = cp;
				Moveto(p);
				break;

			    case RLINETO:
				cp.x += xyvec[0];
				cp.y += xyvec[1];
				if ( Locktype )
					p = LockPoint(cp);
				else p = cp;
				Lineto(p);
				break;

			    case HLINETO:
				cp.x += xyvec[0];
				if ( Locktype )
					p = LockPoint(cp);
				else p = cp;
				Lineto(p);
				break;

			    case VLINETO:
				cp.y += xyvec[0];
				if ( Locktype )
					p = LockPoint(cp);
				else p = cp;
				Lineto(p);
				break;

			    case RRCURVETO:
				xx = xyvec[0];
				yy = xyvec[1];
				p0.x = xx + cp.x;
				p0.y = yy + cp.y;
				if(top > 2)xx = xyvec[2];
				if(top > 3)yy = xyvec[3];
				else yy = 0.;
				p1.x = p0.x + xx;
				p1.y = p0.y + yy;
				if(top > 4)xx = xyvec[4];
				else xx = 0.;
				if(top > 5)yy = xyvec[5];
				else yy = 0.;
				p2.x = p1.x + xx;
				p2.y = p1.y + yy;
				cp = p2;
				if ( Locktype ) {
					p2 = LockPoint(p2);
					p1 = LockPoint(p1);
					p0 = LockPoint(p0);
				}
				add_element(PA_CURVETO, _transform(p0));
				add_element(PA_NONE, _transform(p1));
				add_element(PA_NONE, _transform(p2));
				break;

			    case CLOSEPATH:
				closepathOP();
				break;

			    case CALLSUBR:
				if ( nsource >= 10 )
					pserror("invalidfont", "too many Subr calls");
				if(tdebug)fprintf(stderr,"  calling %d level %d\n",(int)xyvec[top-1],nsource);
				obj = dictget(Private, n_Subrs);
				if ( obj.type != OB_ARRAY )
					pserror("invalidfont", "no Subrs array");
				obj = obj.value.v_array.object[(int)xyvec[--top]];
				if ( obj.type != OB_STRING )
					pserror("invalidfont", "bad Subr entry");
				source[nsource].code = Key;
				source[nsource++].string = CharString;
				Key = SHOWSTART;
				CharString = obj.value.v_string.chars;
				for ( i = 0; i < 4; i++ )
					Newchar();
				goto nextc;	/*changed from break*/

			    case RETURN:
				if ( nsource-- <= 0 )
					pserror("invalidfont", "bad Subr return");
if(tdebug)fprintf(stderr,"returning level %d\n",nsource);
				CharString = source[nsource].string;
				Key = source[nsource].code;
				goto nextc;

			    case ESCAPE:
				d = Newchar();
				if ( tdebug )
					fprintf(stderr, "  subcase %x\n", d);
				switch ( d ) {
				    case DOTSECTION:
					IsRelative = IsRelative ? 0 : 1;
					if ( IsRelative ) {
						p = Lck(cp);
						LastPoint.x = p.x - cp.x;
						LastPoint.y = p.y - cp.y;
					} else LastPoint.x = LastPoint.y = 0.0;
					break;

				    case VSTEM3:
					if(Locktype){
						xyvec[0] += sb.x;
						xyvec[1] += xyvec[0];
						xyvec[2] += sb.x;
						xyvec[3] += xyvec[2];
						xyvec[4] += sb.x;
						xyvec[5] += xyvec[4];
						vec = GetXlckVector(xyvec[0],xyvec[1]);
						InsertLck(vec.tail, vec.tip, XlckTable);
						vec = GetXlckVector(xyvec[2],xyvec[3]);
						InsertLck(vec.tail, vec.tip, XlckTable);
						vec = GetXlckVector(xyvec[4],xyvec[5]);
						InsertLck(vec.tail, vec.tip, XlckTable);
					}
					break;

				    case HSTEM3:
					if(Locktype){
						xyvec[0] += sb.y;
						xyvec[1] += xyvec[0];
						xyvec[2] += sb.y;
						xyvec[3] += xyvec[2];
						xyvec[4] += sb.y;
						xyvec[5] += xyvec[4];
						vec = GetYlckVector(xyvec[0],xyvec[1]);
						InsertLck(vec.tail, vec.tip, YlckTable);
						vec = GetYlckVector(xyvec[2],xyvec[3]);
						InsertLck(vec.tail, vec.tip, YlckTable);
						vec = GetYlckVector(xyvec[4],xyvec[5]);
						InsertLck(vec.tail, vec.tip, YlckTable);
					}
					break;

				    case SEAC:	/*accented char*/
					inseac = 1;
					asb = xyvec[0];
					p.x = xyvec[1] + sb.x;
					p.y = xyvec[2];
					bchar = xyvec[3];
					achar = xyvec[4];
					p0 = Graphics.origin;
					p1 = width;
					ExecCharString(font, bchar, charpath, bool);
					inseac = 2;
					cp.x = p.x;
					cp.y = p.y;
					sb.x = p.x;
					sb.y = 0;
					orx = -asb;
					ExecCharString(font, achar, charpath, bool);
					inseac = 0;
					Graphics.origin = p0;
					width = p1;
					goto endchar;

				    case SBW:	/*set sideb*/
					if(inseac == 2)
						break;
					if ( nMetric <= 1 ) {
						cp.x = xyvec[0];
						cp.y = xyvec[1];
						if(cacheout||merganthal){
							xleft=_transform(cp);
							if(cacheout)fprintf(stderr," %d %f",(int)xleft.x);
							else fprintf(stderr,"xleft %d %f\n",(int)xleft.x,xleft.x);
						}
						dictput(Internaldict, n_sbx, makereal(cp.x));
						dictput(Internaldict, n_sby, makereal(cp.y));
					}
					if ( nMetric == 0 ) {
						width.x = xyvec[2];
						width.y = xyvec[3];
						if((merganthal||cacheout) && !inseac){
							w = _transform(width);
							if(cacheout)fprintf(stderr," %d",(int)w.x);
							else fprintf(stderr,"width %d %f\n",(int)w.x,w.x);
						}
					}
					break;

				    case DIV:
					top--;
					xyvec[top-1] /= xyvec[top];
					goto nextc;

				    case CALLOTHERSUBR:
					obj = dictget(Private, n_OtherSubrs);
					if ( obj.type != OB_ARRAY )
						pserror("invalidfont", "OtherSubrs");
					i = xyvec[--top];
					obj = obj.value.v_array.object[i];
					if ( obj.type != OB_ARRAY )
						pserror("invalidfont", "not a Subr");
					n = xyvec[--top];
					if(n > 0)
						for ( i = 0; i < n; i++ ){
							push(makereal(xyvec[--top]));
						}
					begin(systemdict);
					begin(fontobj);
					execpush(obj);
					execute();
					endOP();
					endOP();
					if(top < 0)top=0;
					goto nextc;

				    case POP:	/*pop*/
					xyvec[top++] = PopDouble();
					goto nextc;

				    case SETCP:	/*setcurrentpoint*/
					break;	/*was goto*/

				    default:
					fprintf(stderr,"missing subcase:0xC %x ",d);
					warning("type1");
				}
				break;

			    case HSBW:	/*hsbw set leftsidebearing & char width & currentpoint*/
				if(inseac==2)
					break;
				if ( nMetric <= 1 ) {
					cp.x = sb.x = xyvec[0];
					cp.y = sb.y = 0.0;
					if((cacheout||merganthal) && !inseac){
						xleft = _transform(sb);
						if(cacheout)fprintf(stderr," %d",(int)xleft.x);
						else fprintf(stderr,"xleft %d %f\n",(int)xleft.x,xleft.x);
					}
					dictput(Internaldict, n_sbx, makereal(sb.x));
					dictput(Internaldict, n_sby, makereal(sb.y));
				}
				if ( nMetric == 0 ) {
					width.x = xyvec[1];
					width.y = 0.0;
					if((merganthal||cacheout) && !inseac){
						w = _transform(width);
						if(cacheout)fprintf(stderr," %d",(int)w.x);
						else fprintf(stderr,"width %d %f\n",(int)w.x,w.x);
					}
				}
				break;

			    case ENDCHAR:	/* far from complete!! endchar */
endchar:			if(inseac)
					return;
				Graphics.width = width;
				close_components() ;
				if(!charpath)FillCharString(issmall);
				else {
					flattenpathOP();
					if(merganthal==1)
						hsbpath(Graphics.path);
					if(bool.value.v_boolean == TRUE && PaintType == 2)
						strokepathOP();
					Graphics.width = dtransform(width);
				}
				if ( tdebug ) DumpLckTables();
				Graphics.incharpath = 0;
				Graphics.CTM[4] = Graphics.origin.x;
				Graphics.CTM[5] = Graphics.origin.y;
				return;

			    case 0xF:	/*undisclosed but used*/
				cp.x = xyvec[0];
				cp.y = xyvec[1];
				if ( Locktype )
					p = LockPoint(cp);
				else p = cp;
				Moveto(p);
				break;

			    case RMOVETO:
				cp.x += xyvec[0];
				cp.y += xyvec[1];
				if ( Locktype )
					p = LockPoint(cp);
				else p = cp;
				Moveto(p);
				break;

			    case HMOVETO:
				cp.x += xyvec[0];
				if ( Locktype )
					p = LockPoint(cp);
				else p = cp;
				Moveto(p);
				break;

			    case VHCURVETO:	/*vhcurveto*/
				p0.x = cp.x;
				p0.y = xyvec[0] + cp.y;
				p1.x = p0.x + xyvec[1];
				p1.y = p0.y + xyvec[2];
				p2.x = p1.x + xyvec[3];
				p2.y = p1.y;
				cp = p2;
				if ( Locktype ) {
					p2 = LockPoint(p2);
					p1 = LockPoint(p1);
					p0 = LockPoint(p0);
				}
				add_element(PA_CURVETO, _transform(p0));
				add_element(PA_NONE, _transform(p1));
				add_element(PA_NONE, _transform(p2));
				break;

			    case HVCURVETO:	/*hvcurveto ~ dx1 0 dx2 dy2 0 dy3 rrcurveto*/
				p0.x = cp.x + xyvec[0];
				p0.y = cp.y;
				p1.x = p0.x + xyvec[1];
				p1.y = p0.y + xyvec[2];
				p2.x = p1.x;
				p2.y = p1.y + xyvec[3];
				cp = p2;
				if ( Locktype ) {
					p2 = LockPoint(p2);
					p1 = LockPoint(p1);
					p0 = LockPoint(p0);
				}
				add_element(PA_CURVETO, _transform(p0));
				add_element(PA_NONE, _transform(p1));
				add_element(PA_NONE, _transform(p2));
				break;

			    default:
				fprintf(stderr, "missing case %x ",c);
				warning("type1");
			}
			top = 0;
		}
nextc:		c = Newchar();
	}

}

void
hsbpath(struct path path)
{
	struct element *p;
	int i=0, x, y;
	if(path.first==NULL){fprintf(stderr,"null\n");return;}
	for(p=path.first; p != NULL; p=p->next){
		x = p->ppoint.x; y = p->ppoint.y;
		fprintf(stderr,"%d %d\n",x,y);
		if(p->next != 0 && p->type == PA_CLOSEPATH)fprintf(stderr,"1e6\n");
	}
	fprintf(stderr,"1e6\n");
}
int
Newchar(void)	/* Recovers and returns the next CharString character.*/
{
	static ct=0;
	int	clear;
	clear = ((Key >> 8) ^ *CharString) & 0xFF;
	Key = (Key + *CharString++) * MAGIC1 + MAGIC2;
	return(clear);
}

void
GetPrivate(struct dict *font)
{
	struct object	obj, num;
	int		i;

	private = dictget(font, n_Private);
	if ( private.type == OB_NONE || private.type != OB_DICTIONARY )
		pserror("invalidfont", "no Private dictionary");
	Private = private.value.v_dict;
	if ( IntLookup(Private, n_password, 0) != FONTPASSWORD )
		pserror("invalidfont", "bad password");
	obj = dictget(Private, n_BlueValues);
	if ( obj.type == OB_ARRAY && (nBlueValues = obj.value.v_array.length) <= 12 ) {
		for ( i = 0; i < nBlueValues; i++ ) {
			num = obj.value.v_array.object[i];
			switch ( num.type ) {
			    case OB_INT:
				BlueValues[i] = num.value.v_int;
				break;
			    case OB_REAL:
				BlueValues[i] = num.value.v_real;
				break;
			    default:
				pserror("invalidfont", "bad BlueValues");
			}
		}
	}
	else pserror("invalidfont", "bad BlueValues");
	BlueFuzz = NumLookup(Private, n_BlueFuzz, BLUEFUZZ);
	BlueShift = NumLookup(Private, n_BlueShift, BLUESHIFT);
	BlueScale = NumLookup(Private, n_BlueScale, BLUESCALE);
}

int
GetMetricInfo(struct dict *font, int code)	/*get metric info from Metrics dict*/
{
	int		nm = -1;
	struct object cs, *encoding;
	struct dict *charstr;

	if(!inseac){
		tEncoding = dictget(font, Encoding);
		encoding = tEncoding.value.v_array.object;
	}
	else encoding = standardencoding.value.v_array.object;
	cs = dictget(font, CharStrings);
	charstr = cs.value.v_dict;
	if(cacheout){
		fprintf(stderr,"%d ", code);
		push(encoding[code]);
		printOP();
	}
	cs = dictget(charstr, encoding[code]);
	if(cs.type != OB_STRING){
		if(mdebug)
			fprintf(stderr,"no charstring for code %d type %d\n",code,cs.type);
		return(-2);
	}
	CharString = cs.value.v_string.chars;
	Metrics = dictget(font, n_Metrics);
	width.x = width.y = 0.;
	if(inseac != 2)sb.x = sb.y = 0.;

	if ( Metrics.type == OB_NONE )
		nm = 0;
	else if ( Metrics.type == OB_DICTIONARY ) {
		Metric = dictget(Metrics.value.v_dict, encoding[code]);
		switch(Metric.type){
		case OB_NONE:
			nm = 0;
			break;
		case OB_INT:
			nm = 1;
			width.x = Metric.value.v_int;
			break;
		case OB_REAL:
			nm = 1;
			width.x = Metric.value.v_real;
			break;
		case OB_ARRAY:
			nm = Metric.value.v_array.length;
			if ( nm == 2 ){
				width.x = ArrayDoubleGet(Metric, 1);
				if(inseac != 2)sb.x = ArrayDoubleGet(Metric, 0);
			}
			if ( nm == 4 ){
				width.x = ArrayDoubleGet(Metric, 2);
				width.y = ArrayDoubleGet(Metric, 3);
				if(inseac != 2){
					sb.x = ArrayDoubleGet(Metric, 0);
					sb.y = ArrayDoubleGet(Metric, 1);
				}
			}
			else pserror("invalid font", "bad Metrics entry");
			break;
		default:
			pserror("invalid font", "bad Metrics entry");
		}
	}
	if(inseac != 2)cp = sb;
	return(nm);
}

int
SetLocktype(void)
{
	int	issmall;

	issmall = fmin(Dnorm(FONTINCH, 0.0), Dnorm(0.0, FONTINCH)) >= SMALLCHAR ? 0 : 1;
	if ( issmall )
		Locktype = 0;
	else if ( fabs(Graphics.CTM[0]) < SMALLCTM || fabs(Graphics.CTM[3]) < SMALLCTM )
		Locktype = -1;
	else if ( fabs(Graphics.CTM[1]) < SMALLCTM || fabs(Graphics.CTM[2]) < SMALLCTM )
		Locktype = 1;
	else Locktype = 0;
	return(issmall);
}


void
InsertLck(double tail, double tip, LckTable *table)	/*insert lock vector in *table*/
{
	int		i, j , count;

	IsLocked = 0;
	count = table[0].count;
	if(count >= LCKTBLSZ)
		pserror("table overflow", "InsertLck");

	table[0].tail = tail - 1.0;
	table[0].tip = tip - 1.0;
	table[count+1].tail = tail + 1.0;
	table[count+1].tip = tip + 1.0;

	for ( i = 1; i <= count; i++ )
		if ( tail <= table[i].tail )
			break;

	if ( table[i].tail == tail || tip < table[i-1].tip || tip > table[i].tip )
		return;
	else if ( tip == table[i-1].tip )
		table[i-1].tail = (tail + table[i-1].tail) / 2.0;
	else if ( tip == table[i].tip )
		table[i].tail = (tail + table[i].tail) / 2.0;
	else {
		for ( j = count + 1; j > i; j-- ) {
			table[j].tail = table[j-1].tail;
			table[j].tip = table[j-1].tip;
			table[j].scale = table[j-1].scale;
		}
		table[i].tail = tail;
		table[i].tip = tip;
		table[0].count++;
	}
}

Vector
GetXlckVector(double x0, double x1)
{
	struct pspoint	middle, length, p;
	Vector	vec;

	middle.x = vec.tail = (x0 + x1) / 2.0;
	middle.y = 0.0;
	middle = _transform(middle);

	length.x = x1 - x0;
	length.y = 0.0;
	length = dtransform(length);

	if ( Locktype < 0 )
		middle.y = LckMiddle(middle.y, length.y);
	else middle.x = LckMiddle(middle.x, length.x);

	middle = itransform(middle);
	vec.tip = middle.x;
	return(vec);
}

Vector
GetYlckVector(double y0, double y1)
{
	int	index, gotpoint;
	double	lckpt, upper, lower, dy, endpoint, length;
	double	shift, bs;
	struct pspoint	p;
	Vector	vec;
	double	temp;
/*
 *
 * Locks the center (as in GetXlckVector()) if the end points don't intersect
 * a blue band, otherwise builds a lock vector that adjusts the center so that
 * the end point locks onto a device space pixel. Checks the lower endpoint in
 * the first band and the upper endpoint in all the others. Definitely needs to
 * be cleaned up!
 *
 */

	if ( y1 < y0 ) {
		temp = y0;
		y0 = y1;
		y1 = temp;
	}
	for ( index = 0; index < nBlueValues; index += 2 ) {
		lower = BlueValues[index];
		upper = BlueValues[index+1];
		endpoint = (index == 0) ? y0 : y1;
		if ( (lower - BlueFuzz) <= endpoint && endpoint <= (upper + BlueFuzz) )
			break;
	}
	if ( index >= nBlueValues )
		return(SimpleYlck(y0, y1));
	if ( fabs(endpoint - lower) <= BlueFuzz )
		endpoint = lower;
	if ( fabs(endpoint - upper) <= BlueFuzz )
		endpoint = upper;
	p.x = 0.0;
	p.y = y1 - y0;
	p = dtransform(p);
	length = BaselineNormal(p);
	gotpoint = 0;
	if ( index != 0 ) {	/* is the band (in device space) too small? */
		p.x = 0.0;
		p.y = lower;
		p = _transform(p);
		if ( abs(Round(BaselineNormal(p))) < abs(Round(BlueScale * upper)) ) {
			p.x = 0.0;
			p.y = upper;
			p = _transform(p);
			lckpt = Round(BaselineNormal(p));
			gotpoint = 1;
		}
	}
	if ( gotpoint == 0 ) {
		p.x = 0.0;
		p.y = (index == 0) ? upper : lower;
		p = _transform(p);
		lckpt = Round(BaselineNormal(p));
		dy = (index == 0) ? upper - endpoint : endpoint - lower;
		if ( dy != 0.0 ) {
			p.x = 0.0;
			p.y = dy;
			p = dtransform(p);
			shift = BaselineNormal(p);
			if ( fabs(dy) >= BlueShift ) {
				bs = fmin(fmax(fabs(dy)*BlueScale - 0.5, -0.499), 0.499);
				shift += (shift >= 0) ? -bs : bs;
			}
			shift = Round(shift);
			lckpt = (index == 0) ? lckpt - shift : lckpt + shift;
		}
	}
	if ( (shift = Round(fabs(length))) == 0.0 && PaintType != 2)
		shift = 1.0;
	shift /= 2.0;
	if ( PaintType == 2)
		shift += 0.5;
	if ( length < 0 )
		shift = -shift;
	shift = (index == 0) ? lckpt + shift : lckpt - shift;
	if ( Locktype < 0 ) {
		p.x = shift;
		p.y = 0.0;
	} else {
		p.x = 0.0;
		p.y = shift;
	}
	p = itransform(p);
	vec.tail = (y0 + y1) / 2.0;
	vec.tip = p.y;
	return(vec);
}

Vector
SimpleYlck(double y0, double y1)
{
	struct pspoint	middle, length;
	Vector	vec;

	middle.x = 0.0;
	middle.y = vec.tail = (y0 + y1) / 2.0;
	middle = _transform(middle);
	length.x = 0;
	length.y = y1 - y0;
	length = dtransform(length);
	if ( Locktype < 0 )
		middle.x = LckMiddle(middle.x, length.x);
	else middle.y = LckMiddle(middle.y, length.y);
	middle = itransform(middle);
	vec.tip = middle.y;
	return(vec);
}

double
LckMiddle(double middle, double length)
{
	int	ilength, imiddle, even;

	if ( ilength = Round(fabs(length)) ) {
		even = 1 - ilength % 2;
		if ( PaintType == 2)
			even = !even;
		if ( even )
			return((double)Round(middle));
	}
	imiddle = middle;
	return(imiddle >= 0 ? imiddle + 0.5 : imiddle - 0.5);
}

struct pspoint
LockPoint(struct pspoint p)
{

	if ( IsRelative ) {
		p.x += LastPoint.x;
		p.y += LastPoint.y;
	} else p = Lck(p);

	return(p);
}

struct pspoint
Lck(struct pspoint p)
{
	if ( unlock == 0 ) {
		if ( XlckTable[0].count > 0 )
			p.x = XYLck(p.x, XlckTable);
		if ( YlckTable[0].count > 0 )
			p.y = XYLck(p.y, YlckTable);
	}
	return(p);
}

double
XYLck(double number, LckTable *table)
{
	int		count, i;

	count = table[0].count;
	if ( count == 1 || number <= table[1].tail )
		return(number + table[1].tip - table[1].tail);
	if ( number >= table[count].tail )
		return(number + table[count].tip - table[count].tail);
	if ( IsLocked == 0 )
		LckTableInit();
	for ( i = 1; i < count; i++ )
		if ( table[i].tail <= number && number <= table[i+1].tail )
			break;
	return(table[i].scale * (number - table[i].tail) + table[i].tip);
}

void
LckTableInit(void)	/*may not be right*/
{
	int	i;

	for ( i = 1; i < XlckTable[0].count; i++ )
		XlckTable[i].scale = (XlckTable[i+1].tip - XlckTable[i].tip) /
				       (XlckTable[i+1].tail - XlckTable[i].tail);
	for ( i = 1; i < YlckTable[0].count; i++ )
		YlckTable[i].scale = (YlckTable[i+1].tip - YlckTable[i].tip) /
				       (YlckTable[i+1].tail - YlckTable[i].tail);
	IsLocked = 1;
}

void
DumpLckTables(void)
{
	int	i;

	fprintf(stderr, "XlckTable size = %d\n", XlckTable[0].count);
	for ( i = 1; i < XlckTable[0].count+1; i++ )
		fprintf(stderr, " tail=%g tip=%g scale=%g\n", XlckTable[i].tail,
					XlckTable[i].tip, XlckTable[i].scale);
	fprintf(stderr, "YlckTable size = %d\n", YlckTable[0].count);
	for ( i = 1; i < YlckTable[0].count+1; i++ )
		fprintf(stderr, " tail=%g tip=%g scale=%g\n", YlckTable[i].tail,
					YlckTable[i].tip, YlckTable[i].scale);
}

/*
 *
 * Everything that follows is an attempt to isolate psi specific routine calls.
 * Got most (but not all) and I'm quitting for now.
 *
 */


double
PopDouble(void)
{
	struct object	obj;

	obj = pop();
	switch ( obj.type ) {
	case OB_INT: return((double)obj.value.v_int);
	case OB_REAL: return((double)obj.value.v_real);
	default: pserror("typecheck", "PopDouble");
	}
}

int
IntLookup(struct dict *dict, struct object name, int missing)
{
	struct object	obj;

	obj = dictget(dict, name);
	if ( obj.type == OB_INT )
		return(obj.value.v_int);
	 else return(missing);
}

double
NumLookup(struct dict *dict, struct object name, double missing)
{
	struct object	obj;

	obj = dictget(dict, name);
	switch ( obj.type ) {
	case OB_INT: return((double)obj.value.v_int);
	case OB_REAL: return((double)obj.value.v_real);
	default: return(missing);
	}
}


double
ArrayDoubleGet(struct object obj, int index)
{
	if ( obj.type != OB_ARRAY )
		pserror("typecheck", "ArrayDoubleGet");

	obj = obj.value.v_array.object[index];
	switch ( obj.type ) {
	case OB_INT: return((double)obj.value.v_int);
	case OB_REAL: return((double)obj.value.v_real);
	default: pserror("typecheck", "ArrayDoubleGet");
	}
}

double
Dnorm(double x, double y)
{
	struct pspoint	p;

	p.x = x;
	p.y = y;
	p = dtransform(p);
	return(sqrt(p.x*p.x + p.y*p.y));
}

void
FillCharString(int issmall)
{
	flattenpathOP() ;
	pushpoint(width);
	if(!dontcache){
		pathbboxOP();
		setcachedeviceOP();
	}
	else setcharwidthOP();
	if(Graphics.clipchanged){
		do_clip();
	}
	if(PaintType == 2){
		Graphics.linewidth = StrokeWidth;
		strokepathOP();
		sdo_fill(fillOP);
	}
	else sdo_fill(fillOP) ;
	newpathOP() ;
}


int
df_getc(FILE *fp)
{
	register int c=0, c1,i;
	unsigned int xkey;
/*	if(instring)pserror("instring set in df_getc", "internal");*/
	if(d_unget != EOF){
		c = d_unget;
		d_unget = EOF;
		return(c);
	}
	if(decryptf == 1){
	for(i=0;i<2;){
		c1 = fgetc(fp);
		if(c1 == EOF)return(c1);
		if(isascii(c1) && isxdigit(c1)){
			c <<= 4;
			if(isdigit(c1))

				c |= c1 - '0';
			else if(isupper(c1))
				c |= c1 - 'A' + 10;
			else c |= c1 - 'a' + 10;
			i++;
		}
	}
	}
	else c = f_getc(fp)&0377;
	xkey=d_key;
	d_key = (xkey + c) * MAGIC1 + MAGIC2;
	c = ((xkey >>8) ^ c) & 0xFF;
	return(c);
}
extern char gindex;
static Bitmap *bm;
static int cachex, fixx;

#define DY1	1.0
#define YSTEP	DY1
void
sdo_fill(void (*caller)(void))
{
	struct element	*p ;
	struct x	x[PATH_LIMIT];		/* x intersections etc. */
	int		nx;			/* next slot in x[] */
	double		y, ry;			/* next raster line */
	double		Fminx, Fminy;		/* path bounding box */
	double		Fmaxx, Fmaxy;
	double		left, right;
	double		tx, ty, sx, sy;
	int		i, winding, fixy, sentone, height, chno ;

/*
 *
 * Modified fill routine. The original version omitted too much and did a poor
 * job filling small paths. What's here is better, but there's still much room
 * for improvement.
 *
 * Completely covers device space with horizontal strips. A path segment that
 * crosses a strip is assigned a direction value of -2 or +2. Segements with one
 * end point in the strip are given a direction of -1 or +1. Segements with both
 * end points in the strip get a direction of 0. Both fill and eofill use the
 * direction field to locate intervals that are to be filled.
 *
 * Character paths are built at the origin after the translation components in
 * the CTM are zeroed out. The original values are saved in the graphics state
 * and retrieved here in incharpath is set.
 *
 */
	if ( Graphics.path.first == NULL )
		return ;
	if(Graphics.device.width == 0){
		cacheit=0;
		return;
	}
	if ( Graphics.incharpath && cacheit<=0 ) {
		tx = Graphics.origin.x;
		ty = Graphics.origin.y;
	}
	else if(cacheit>0){
		tx = fabs(currentcache->origin.x);
		if(currentcache->upper.y < 0 && currentcache->upper.x < 0)
			ty=fabs(floor(currentcache->origin.y));
		else ty = fabs(ceil(currentcache->origin.y));
		height = currentcache->cheight;
		chno = currentcache->charno;
	} else tx = ty = 0;
	if(current_type == 3 && cacheit > 0){
		Graphics.origin.x = sx = savex;
		Graphics.origin.y = sy = savey;
	}
	else { sx = 0.; sy = 0.; }
	gindex = floor(currentgray()*((double)GRAYLEVELS-1.)/51.2);
	if(gindex < 0)gindex = 0;
	if(gindex > 4)gindex = 4;
	if(cacheit>0){
		bm = currentcache->charbits;
		cachex = chno*currentcache->cwidth;
	}
	else {
		bm = &screen;
		cachex = 0;
	}
	Fminx = Fmaxx = Graphics.path.first->ppoint.x - sx;
	Fminy = Fmaxy = Graphics.path.first->ppoint.y - sy;
	for ( p=Graphics.path.first ; p!=NULL ; p=p->next ) {
		p->ppoint.y -= sy;
		p->ppoint.x -= sx;
		if ( p->ppoint.y > Fmaxy )
			Fmaxy = p->ppoint.y;
		if ( p->ppoint.y < Fminy )
			Fminy = p->ppoint.y;
		if ( p->ppoint.x > Fmaxx )
			Fmaxx = p->ppoint.x;
		if ( p->ppoint.x < Fminx )
			Fminx = p->ppoint.x;
	}
	Fminy = floor(Fminy);
	Fmaxy = ceil(Fmaxy) + DY1;
	fixx = sentone = 0;
	for ( y = Fminy; y <= Fmaxy; y += YSTEP ) {
		nx = 0;
		fixy = (int)floor(y + ty);
		if(cacheit > 0){
			if(currentcache->upper.y < 0.){
				fixy = ceil(y - currentcache->upper.y);
				if(currentcache->upper.x < 0.)
					fixy = ceil(y - currentcache->upper.y-ty+1);
			}
		}
		if(cacheit<=0 && (fixy <= Graphics.iminy || fixy+1 >Graphics.imaxy))
			continue;
		sentone=1;
		ry = y + DY1;
		for ( p = Graphics.path.first; p != NULL; p = p->next ) {
			if ( p->type == PA_MOVETO )
				continue;
			if ( p->prev->ppoint.y < y && p->ppoint.y < y ||
			    p->prev->ppoint.y >= ry && p->ppoint.y >= ry )
				continue;
			x[nx++] = intercepts(p->prev, p, y);
		}
		qsort(x,nx,sizeof(*x),sxcomp) ;
		if ( caller == (void (*)(void))eofillOP )
			for ( i=0 ; i<nx ; ){
				winding=0;
				left = x[i].left;
				right = x[i].right;
				do {
					if(x[i].direction == -2)
						winding += 2;
					else winding += x[i].direction;
					if(left > x[i].left)
						left = x[i].left;
					if(right < x[i].right)
						right = x[i].right;
					i++;
				} while(winding != 0 && winding != 4 && i < nx);
			if(cacheit > 0 && currentcache->upper.x < 0){
			       spaint((int)floor(left-currentcache->upper.x),
				(int)floor(right-currentcache->upper.x),fixy);
			}
			else spaint((int)floor(left+tx), (int)floor(right+tx),fixy) ;
			}
		else {
		for ( i=0 ; i<nx ; ) {
			winding = 0;
			left = x[i].left;
			right = x[i].right;
			do {
				winding += x[i].direction ;
				if ( left > x[i].left )
					left = x[i].left;
				if ( right < x[i].right )
					right = x[i].right;
				i++;
			} while ( winding != 0 && i < nx );
			if(cacheit > 0 && currentcache->upper.x < 0){
			       spaint((int)floor(left-currentcache->upper.x),
				(int)floor(right-currentcache->upper.x),fixy);
			}
			else spaint((int)floor(left+tx), (int)floor(right+tx),fixy) ;
		}
		}
	}
	if(cacheit>0){
		if(cacheout){
			if(xleft.x < 0.)fprintf(stderr," %d\n",cachex+(int)(tx + xleft.x));
			else fprintf(stderr," %d\n",cachex+(int)tx);
		}
		currentcache->cachec[chno].edges.min.x = cachex;
		currentcache->cachec[chno].edges.min.y = 0;
		currentcache->cachec[chno].edges.max.x = cachex +currentcache->cwidth+1;
		currentcache->cachec[chno].edges.max.y = height+1;
		ty = currentcache->origin.y;
		yadj = nxadj = 0.;
#ifndef Plan9
		y = Graphics.device.height - (Graphics.origin.y + anchor.y -ty);
#else
		y = Graphics.device.height - Graphics.origin.y + anchor.y + ty;
#endif
		if(currentcache->upper.y < 0.){
			yadj = ty;
			y -= (currentcache->upper.y+ty);
			if(currentcache->upper.x<0)y -= ty;
		}
		if(currentcache->upper.x < 0){
			if(currentcache->upper.y > 0.)tx = - currentcache->upper.x;
			else {
				yadj = ty;
				nxadj = tx;
				tx = - currentcache->upper.x;
			}
		}
		fixy = (int)floor(y-height);
		bitblt(&screen,Pt((int)floor(Graphics.origin.x-tx+anchor.x),fixy),
			bm,currentcache->cachec[chno].edges, S|D);
#ifndef Plan9
		ckbdry((int)ceil(Graphics.origin.x-tx),fixy);
		ckbdry((int)ceil(Graphics.origin.x+currentcache->cwidth),fixy+2*height);
#endif
	}
}
void
spaint(int x1, int x2, int  y)
{
	int yf;
	if(cacheit<=0){
		if ( x2 < Graphics.iminx  ||  x1 > Graphics.imaxx )
			return ;
		if ( x2 >= Graphics.imaxx )
			x2 = Graphics.imaxx;
	}
	if ( x1 < 0 )x1 = 0 ;
	if(x2 < 0)x2 = 0;
	if(cacheit>0)if(x2 > fixx)fixx=x2;

	if(cacheit<=0){
		y = Graphics.device.height - y + anchor.y;
		x1 += anchor.x;
		x2 += anchor.x;
#ifndef Plan9
		ckbdry(x1,y); ckbdry(x2,y+1);
#endif
	}
	else {
		y = currentcache->cheight - y;
		if(y>currentcache->cheight){
			if(mdebug)fprintf(stderr,"y wrapping %d\n",y);
			 y-= currentcache->cheight;
		}
	}
	if(x1 == x2)x2++;
	texture(bm,Rect(x1+cachex,y,x2+cachex,y+1),pgrey[gindex],S);
}

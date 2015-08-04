/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <ctype.h>
#include <html.h>
#include "impl.h"

typedef struct TokenSource TokenSource;
struct TokenSource
{
	int			i;		// index of next byte to use
	uint8_t*		data;		// all the data
	int			edata;	// data[0:edata] is valid
	int			chset;	// one of US_Ascii, etc.
	int			mtype;	// TextHtml or TextPlain
};

enum {
	EOF = -2,
	EOB = -1
};

#define ISNAMCHAR(c)	((c)<256 && (isalpha(c) || isdigit(c) || (c) == '-' || (c) == '.'))

#define SMALLBUFSIZE 240
#define BIGBUFSIZE 2000

// HTML 4.0 tag names.
// Keep sorted, and in correspondence with enum in iparse.h.
Rune* tagnames[] = {
	(Rune*)L" ",
	(Rune*)L"!",
	(Rune*)L"a", 
	(Rune*)L"abbr",
	(Rune*)L"acronym",
	(Rune*)L"address",
	(Rune*)L"applet", 
	(Rune*)L"area",
	(Rune*)L"b",
	(Rune*)L"base",
	(Rune*)L"basefont",
	(Rune*)L"bdo",
	(Rune*)L"big",
	(Rune*)L"blink",
	(Rune*)L"blockquote",
	(Rune*)L"body",
	(Rune*)L"bq",
	(Rune*)L"br",
	(Rune*)L"button",
	(Rune*)L"caption",
	(Rune*)L"center",
	(Rune*)L"cite",
	(Rune*)L"code",
	(Rune*)L"col",
	(Rune*)L"colgroup",
	(Rune*)L"dd",
	(Rune*)L"del",
	(Rune*)L"dfn",
	(Rune*)L"dir",
	(Rune*)L"div",
	(Rune*)L"dl",
	(Rune*)L"dt",
	(Rune*)L"em",
	(Rune*)L"fieldset",
	(Rune*)L"font",
	(Rune*)L"form",
	(Rune*)L"frame",
	(Rune*)L"frameset",
	(Rune*)L"h1",
	(Rune*)L"h2",
	(Rune*)L"h3",
	(Rune*)L"h4",
	(Rune*)L"h5",
	(Rune*)L"h6",
	(Rune*)L"head",
	(Rune*)L"hr",
	(Rune*)L"html",
	(Rune*)L"i",
	(Rune*)L"iframe",
	(Rune*)L"img",
	(Rune*)L"input",
	(Rune*)L"ins",
	(Rune*)L"isindex",
	(Rune*)L"kbd",
	(Rune*)L"label",
	(Rune*)L"legend",
	(Rune*)L"li",
	(Rune*)L"link",
	(Rune*)L"map",
	(Rune*)L"menu",
	(Rune*)L"meta",
	(Rune*)L"nobr",
	(Rune*)L"noframes",
	(Rune*)L"noscript",
	(Rune*)L"object",
	(Rune*)L"ol",
	(Rune*)L"optgroup",
	(Rune*)L"option",
	(Rune*)L"p",
	(Rune*)L"param",
	(Rune*)L"pre",
	(Rune*)L"q",
	(Rune*)L"s",
	(Rune*)L"samp",
	(Rune*)L"script",
	(Rune*)L"select",
	(Rune*)L"small",
	(Rune*)L"span",
	(Rune*)L"strike",
	(Rune*)L"strong",
	(Rune*)L"style",
	(Rune*)L"sub",
	(Rune*)L"sup",
	(Rune*)L"table",
	(Rune*)L"tbody",
	(Rune*)L"td",
	(Rune*)L"textarea",
	(Rune*)L"tfoot",
	(Rune*)L"th",
	(Rune*)L"thead",
	(Rune*)L"title",
	(Rune*)L"tr",
	(Rune*)L"tt",
	(Rune*)L"u",
	(Rune*)L"ul",
	(Rune*)L"var"
};

// HTML 4.0 attribute names.
// Keep sorted, and in correspondence with enum in impl.h.
Rune* attrnames[] = {
	(Rune*)L"abbr",
	(Rune*)L"accept-charset",
	(Rune*)L"access-key",
	(Rune*)L"action",
	(Rune*)L"align",
	(Rune*)L"alink",
	(Rune*)L"alt",
	(Rune*)L"archive",
	(Rune*)L"axis",
	(Rune*)L"background",
	(Rune*)L"bgcolor",
	(Rune*)L"border",
	(Rune*)L"cellpadding",
	(Rune*)L"cellspacing",
	(Rune*)L"char",
	(Rune*)L"charoff",
	(Rune*)L"charset",
	(Rune*)L"checked",
	(Rune*)L"cite",
	(Rune*)L"class",
	(Rune*)L"classid",
	(Rune*)L"clear",
	(Rune*)L"code",
	(Rune*)L"codebase",
	(Rune*)L"codetype",
	(Rune*)L"color",
	(Rune*)L"cols",
	(Rune*)L"colspan",
	(Rune*)L"compact",
	(Rune*)L"content",
	(Rune*)L"coords",
	(Rune*)L"data",
	(Rune*)L"datetime",
	(Rune*)L"declare",
	(Rune*)L"defer",
	(Rune*)L"dir",
	(Rune*)L"disabled",
	(Rune*)L"enctype",
	(Rune*)L"face",
	(Rune*)L"for",
	(Rune*)L"frame",
	(Rune*)L"frameborder",
	(Rune*)L"headers",
	(Rune*)L"height",
	(Rune*)L"href",
	(Rune*)L"hreflang",
	(Rune*)L"hspace",
	(Rune*)L"http-equiv",
	(Rune*)L"id",
	(Rune*)L"ismap",
	(Rune*)L"label",
	(Rune*)L"lang",
	(Rune*)L"link",
	(Rune*)L"longdesc",
	(Rune*)L"marginheight",
	(Rune*)L"marginwidth",
	(Rune*)L"maxlength",
	(Rune*)L"media",
	(Rune*)L"method",
	(Rune*)L"multiple",
	(Rune*)L"name",
	(Rune*)L"nohref",
	(Rune*)L"noresize",
	(Rune*)L"noshade",
	(Rune*)L"nowrap",
	(Rune*)L"object",
	(Rune*)L"onblur",
	(Rune*)L"onchange",
	(Rune*)L"onclick",
	(Rune*)L"ondblclick",
	(Rune*)L"onfocus",
	(Rune*)L"onkeypress",
	(Rune*)L"onkeyup",
	(Rune*)L"onload",
	(Rune*)L"onmousedown",
	(Rune*)L"onmousemove",
	(Rune*)L"onmouseout",
	(Rune*)L"onmouseover",
	(Rune*)L"onmouseup",
	(Rune*)L"onreset",
	(Rune*)L"onselect",
	(Rune*)L"onsubmit",
	(Rune*)L"onunload",
	(Rune*)L"profile",
	(Rune*)L"prompt",
	(Rune*)L"readonly",
	(Rune*)L"rel",
	(Rune*)L"rev",
	(Rune*)L"rows",
	(Rune*)L"rowspan",
	(Rune*)L"rules",
	(Rune*)L"scheme",
	(Rune*)L"scope",
	(Rune*)L"scrolling",
	(Rune*)L"selected",
	(Rune*)L"shape",
	(Rune*)L"size",
	(Rune*)L"span",
	(Rune*)L"src",
	(Rune*)L"standby",
	(Rune*)L"start",
	(Rune*)L"style",
	(Rune*)L"summary",
	(Rune*)L"tabindex",
	(Rune*)L"target",
	(Rune*)L"text",
	(Rune*)L"title",
	(Rune*)L"type",
	(Rune*)L"usemap",
	(Rune*)L"valign",
	(Rune*)L"value",
	(Rune*)L"valuetype",
	(Rune*)L"version",
	(Rune*)L"vlink",
	(Rune*)L"vspace",
	(Rune*)L"width"
};


// Character entity to unicode character number map.
// Keep sorted by name.
StringInt	chartab[]= {
	{(Rune*)L"AElig", 198},
	{(Rune*)L"Aacute", 193},
	{(Rune*)L"Acirc", 194},
	{(Rune*)L"Agrave", 192},
	{(Rune*)L"Alpha", 913},
	{(Rune*)L"Aring", 197},
	{(Rune*)L"Atilde", 195},
	{(Rune*)L"Auml", 196},
	{(Rune*)L"Beta", 914},
	{(Rune*)L"Ccedil", 199},
	{(Rune*)L"Chi", 935},
	{(Rune*)L"Dagger", 8225},
	{(Rune*)L"Delta", 916},
	{(Rune*)L"ETH", 208},
	{(Rune*)L"Eacute", 201},
	{(Rune*)L"Ecirc", 202},
	{(Rune*)L"Egrave", 200},
	{(Rune*)L"Epsilon", 917},
	{(Rune*)L"Eta", 919},
	{(Rune*)L"Euml", 203},
	{(Rune*)L"Gamma", 915},
	{(Rune*)L"Iacute", 205},
	{(Rune*)L"Icirc", 206},
	{(Rune*)L"Igrave", 204},
	{(Rune*)L"Iota", 921},
	{(Rune*)L"Iuml", 207},
	{(Rune*)L"Kappa", 922},
	{(Rune*)L"Lambda", 923},
	{(Rune*)L"Mu", 924},
	{(Rune*)L"Ntilde", 209},
	{(Rune*)L"Nu", 925},
	{(Rune*)L"OElig", 338},
	{(Rune*)L"Oacute", 211},
	{(Rune*)L"Ocirc", 212},
	{(Rune*)L"Ograve", 210},
	{(Rune*)L"Omega", 937},
	{(Rune*)L"Omicron", 927},
	{(Rune*)L"Oslash", 216},
	{(Rune*)L"Otilde", 213},
	{(Rune*)L"Ouml", 214},
	{(Rune*)L"Phi", 934},
	{(Rune*)L"Pi", 928},
	{(Rune*)L"Prime", 8243},
	{(Rune*)L"Psi", 936},
	{(Rune*)L"Rho", 929},
	{(Rune*)L"Scaron", 352},
	{(Rune*)L"Sigma", 931},
	{(Rune*)L"THORN", 222},
	{(Rune*)L"Tau", 932},
	{(Rune*)L"Theta", 920},
	{(Rune*)L"Uacute", 218},
	{(Rune*)L"Ucirc", 219},
	{(Rune*)L"Ugrave", 217},
	{(Rune*)L"Upsilon", 933},
	{(Rune*)L"Uuml", 220},
	{(Rune*)L"Xi", 926},
	{(Rune*)L"Yacute", 221},
	{(Rune*)L"Yuml", 376},
	{(Rune*)L"Zeta", 918},
	{(Rune*)L"aacute", 225},
	{(Rune*)L"acirc", 226},
	{(Rune*)L"acute", 180},
	{(Rune*)L"aelig", 230},
	{(Rune*)L"agrave", 224},
	{(Rune*)L"alefsym", 8501},
	{(Rune*)L"alpha", 945},
	{(Rune*)L"amp", 38},
	{(Rune*)L"and", 8743},
	{(Rune*)L"ang", 8736},
	{(Rune*)L"aring", 229},
	{(Rune*)L"asymp", 8776},
	{(Rune*)L"atilde", 227},
	{(Rune*)L"auml", 228},
	{(Rune*)L"bdquo", 8222},
	{(Rune*)L"beta", 946},
	{(Rune*)L"brvbar", 166},
	{(Rune*)L"bull", 8226},
	{(Rune*)L"cap", 8745},
	{(Rune*)L"ccedil", 231},
	{(Rune*)L"cdots", 8943},
	{(Rune*)L"cedil", 184},
	{(Rune*)L"cent", 162},
	{(Rune*)L"chi", 967},
	{(Rune*)L"circ", 710},
	{(Rune*)L"clubs", 9827},
	{(Rune*)L"cong", 8773},
	{(Rune*)L"copy", 169},
	{(Rune*)L"crarr", 8629},
	{(Rune*)L"cup", 8746},
	{(Rune*)L"curren", 164},
	{(Rune*)L"dArr", 8659},
	{(Rune*)L"dagger", 8224},
	{(Rune*)L"darr", 8595},
	{(Rune*)L"ddots", 8945},
	{(Rune*)L"deg", 176},
	{(Rune*)L"delta", 948},
	{(Rune*)L"diams", 9830},
	{(Rune*)L"divide", 247},
	{(Rune*)L"eacute", 233},
	{(Rune*)L"ecirc", 234},
	{(Rune*)L"egrave", 232},
	{(Rune*)L"emdash", 8212},	/* non-standard but commonly used */
	{(Rune*)L"empty", 8709},
	{(Rune*)L"emsp", 8195},
	{(Rune*)L"endash", 8211},	/* non-standard but commonly used */
	{(Rune*)L"ensp", 8194},
	{(Rune*)L"epsilon", 949},
	{(Rune*)L"equiv", 8801},
	{(Rune*)L"eta", 951},
	{(Rune*)L"eth", 240},
	{(Rune*)L"euml", 235},
	{(Rune*)L"euro", 8364},
	{(Rune*)L"exist", 8707},
	{(Rune*)L"fnof", 402},
	{(Rune*)L"forall", 8704},
	{(Rune*)L"frac12", 189},
	{(Rune*)L"frac14", 188},
	{(Rune*)L"frac34", 190},
	{(Rune*)L"frasl", 8260},
	{(Rune*)L"gamma", 947},
	{(Rune*)L"ge", 8805},
	{(Rune*)L"gt", 62},
	{(Rune*)L"hArr", 8660},
	{(Rune*)L"harr", 8596},
	{(Rune*)L"hearts", 9829},
	{(Rune*)L"hellip", 8230},
	{(Rune*)L"iacute", 237},
	{(Rune*)L"icirc", 238},
	{(Rune*)L"iexcl", 161},
	{(Rune*)L"igrave", 236},
	{(Rune*)L"image", 8465},
	{(Rune*)L"infin", 8734},
	{(Rune*)L"int", 8747},
	{(Rune*)L"iota", 953},
	{(Rune*)L"iquest", 191},
	{(Rune*)L"isin", 8712},
	{(Rune*)L"iuml", 239},
	{(Rune*)L"kappa", 954},
	{(Rune*)L"lArr", 8656},
	{(Rune*)L"lambda", 955},
	{(Rune*)L"lang", 9001},
	{(Rune*)L"laquo", 171},
	{(Rune*)L"larr", 8592},
	{(Rune*)L"lceil", 8968},
	{(Rune*)L"ldots", 8230},
	{(Rune*)L"ldquo", 8220},
	{(Rune*)L"le", 8804},
	{(Rune*)L"lfloor", 8970},
	{(Rune*)L"lowast", 8727},
	{(Rune*)L"loz", 9674},
	{(Rune*)L"lrm", 8206},
	{(Rune*)L"lsaquo", 8249},
	{(Rune*)L"lsquo", 8216},
	{(Rune*)L"lt", 60},
	{(Rune*)L"macr", 175},
	{(Rune*)L"mdash", 8212},
	{(Rune*)L"micro", 181},
	{(Rune*)L"middot", 183},
	{(Rune*)L"minus", 8722},
	{(Rune*)L"mu", 956},
	{(Rune*)L"nabla", 8711},
	{(Rune*)L"nbsp", 160},
	{(Rune*)L"ndash", 8211},
	{(Rune*)L"ne", 8800},
	{(Rune*)L"ni", 8715},
	{(Rune*)L"not", 172},
	{(Rune*)L"notin", 8713},
	{(Rune*)L"nsub", 8836},
	{(Rune*)L"ntilde", 241},
	{(Rune*)L"nu", 957},
	{(Rune*)L"oacute", 243},
	{(Rune*)L"ocirc", 244},
	{(Rune*)L"oelig", 339},
	{(Rune*)L"ograve", 242},
	{(Rune*)L"oline", 8254},
	{(Rune*)L"omega", 969},
	{(Rune*)L"omicron", 959},
	{(Rune*)L"oplus", 8853},
	{(Rune*)L"or", 8744},
	{(Rune*)L"ordf", 170},
	{(Rune*)L"ordm", 186},
	{(Rune*)L"oslash", 248},
	{(Rune*)L"otilde", 245},
	{(Rune*)L"otimes", 8855},
	{(Rune*)L"ouml", 246},
	{(Rune*)L"para", 182},
	{(Rune*)L"part", 8706},
	{(Rune*)L"permil", 8240},
	{(Rune*)L"perp", 8869},
	{(Rune*)L"phi", 966},
	{(Rune*)L"pi", 960},
	{(Rune*)L"piv", 982},
	{(Rune*)L"plusmn", 177},
	{(Rune*)L"pound", 163},
	{(Rune*)L"prime", 8242},
	{(Rune*)L"prod", 8719},
	{(Rune*)L"prop", 8733},
	{(Rune*)L"psi", 968},
	{(Rune*)L"quad", 8193},
	{(Rune*)L"quot", 34},
	{(Rune*)L"rArr", 8658},
	{(Rune*)L"radic", 8730},
	{(Rune*)L"rang", 9002},
	{(Rune*)L"raquo", 187},
	{(Rune*)L"rarr", 8594},
	{(Rune*)L"rceil", 8969},
	{(Rune*)L"rdquo", 8221},
	{(Rune*)L"real", 8476},
	{(Rune*)L"reg", 174},
	{(Rune*)L"rfloor", 8971},
	{(Rune*)L"rho", 961},
	{(Rune*)L"rlm", 8207},
	{(Rune*)L"rsaquo", 8250},
	{(Rune*)L"rsquo", 8217},
	{(Rune*)L"sbquo", 8218},
	{(Rune*)L"scaron", 353},
	{(Rune*)L"sdot", 8901},
	{(Rune*)L"sect", 167},
	{(Rune*)L"shy", 173},
	{(Rune*)L"sigma", 963},
	{(Rune*)L"sigmaf", 962},
	{(Rune*)L"sim", 8764},
	{(Rune*)L"sp", 8194},
	{(Rune*)L"spades", 9824},
	{(Rune*)L"sub", 8834},
	{(Rune*)L"sube", 8838},
	{(Rune*)L"sum", 8721},
	{(Rune*)L"sup", 8835},
	{(Rune*)L"sup1", 185},
	{(Rune*)L"sup2", 178},
	{(Rune*)L"sup3", 179},
	{(Rune*)L"supe", 8839},
	{(Rune*)L"szlig", 223},
	{(Rune*)L"tau", 964},
	{(Rune*)L"there4", 8756},
	{(Rune*)L"theta", 952},
	{(Rune*)L"thetasym", 977},
	{(Rune*)L"thinsp", 8201},
	{(Rune*)L"thorn", 254},
	{(Rune*)L"tilde", 732},
	{(Rune*)L"times", 215},
	{(Rune*)L"trade", 8482},
	{(Rune*)L"uArr", 8657},
	{(Rune*)L"uacute", 250},
	{(Rune*)L"uarr", 8593},
	{(Rune*)L"ucirc", 251},
	{(Rune*)L"ugrave", 249},
	{(Rune*)L"uml", 168},
	{(Rune*)L"upsih", 978},
	{(Rune*)L"upsilon", 965},
	{(Rune*)L"uuml", 252},
	{(Rune*)L"varepsilon", 8712},
	{(Rune*)L"varphi", 981},
	{(Rune*)L"varpi", 982},
	{(Rune*)L"varrho", 1009},
	{(Rune*)L"vdots", 8942},
	{(Rune*)L"vsigma", 962},
	{(Rune*)L"vtheta", 977},
	{(Rune*)L"weierp", 8472},
	{(Rune*)L"xi", 958},
	{(Rune*)L"yacute", 253},
	{(Rune*)L"yen", 165},
	{(Rune*)L"yuml", 255},
	{(Rune*)L"zeta", 950},
	{(Rune*)L"zwj", 8205},
	{(Rune*)L"zwnj", 8204}
};
#define NCHARTAB (sizeof(chartab)/sizeof(chartab[0]))

// Characters Winstart..Winend are those that Windows
// uses interpolated into the Latin1 set.
// They aren't supposed to appear in HTML, but they do....
enum {
	Winstart = 127,
	Winend = 159
};

static int	winchars[]= { 8226,	// 8226 is a bullet
	8226, 8226, 8218, 402, 8222, 8230, 8224, 8225,
	710, 8240, 352, 8249, 338, 8226, 8226, 8226,
	8226, 8216, 8217, 8220, 8221, 8226, 8211, 8212,
	732, 8482, 353, 8250, 339, 8226, 8226, 376};

static StringInt*	tagtable;		// initialized from tagnames
static StringInt*	attrtable;		// initialized from attrnames

static void	lexinit(void);
static int		getplaindata(TokenSource* ts, Token* a, int* pai);
static int		getdata(TokenSource* ts, int firstc, int starti, Token* a, int* pai);
static int		getscriptdata(TokenSource* ts, int firstc, int starti, Token* a, int* pai, int findtag);
static int		gettag(TokenSource* ts, int starti, Token* a, int* pai);
static Rune*	buftostr(Rune* s, Rune* buf, int j);
static int		comment(TokenSource* ts);
static int		findstr(TokenSource* ts, Rune* s);
static int		ampersand(TokenSource* ts);
static int		getchar(TokenSource* ts);
static void		ungetchar(TokenSource* ts, int c);
static void		backup(TokenSource* ts, int savei);
static void		freeattrs(Attr* ahead);
static Attr*	newattr(int attid, Rune* value, Attr* link);
static int		Tconv(Fmt* f);

int	dbglex = 0;
static int lexinited = 0;

static void
lexinit(void)
{
	tagtable = _makestrinttab(tagnames, Numtags);
	attrtable = _makestrinttab(attrnames, Numattrs);
	fmtinstall('T', Tconv);
	lexinited = 1;
}

static TokenSource*
newtokensource(uint8_t* data, int edata, int chset, int mtype)
{
	TokenSource*	ans;

	assert(chset == US_Ascii || chset == ISO_8859_1 ||
			chset == UTF_8 || chset == Unicode);
	ans = (TokenSource*)emalloc(sizeof(TokenSource));
	ans->i = 0;
	ans->data = data;
	ans->edata = edata;
	ans->chset = chset;
	ans->mtype = mtype;
	return ans;
}

enum {
	ToksChunk = 500,
};

// Call this to get the tokens.
//  The number of returned tokens is returned in *plen.
Token*
_gettoks(uint8_t* data, int datalen, int chset, int mtype, int* plen)
{
	TokenSource*	ts;
	Token*		a;
	int	alen;
	int	ai;
	int	starti;
	int	c;
	int	tag;

	if(!lexinited)
		lexinit();
	ts = newtokensource(data, datalen, chset, mtype);
	if(dbglex)
		fprint(2, "_gettoks starts, ts.i=%d, ts.edata=%d\n", ts->i, ts->edata);
	alen = 0;
	ai = 0;
	a = 0;
	if(ts->mtype == TextHtml) {
		for(;;) {
			if(alen - ai < ToksChunk/32) {
				alen += ToksChunk;
				a = erealloc(a, alen*sizeof *a);
			}
			starti = ts->i;
			c = getchar(ts);
			if(c < 0)
				break;
			if(c == '<') {
				tag = gettag(ts, starti, a, &ai);
				if(tag == Tscript || tag == Tstyle) {
					// special rules for getting Data after....
					starti = ts->i;
					c = getchar(ts);
					tag = getscriptdata(ts, c, starti, a, &ai, tag);
				}
			}
			else
				tag = getdata(ts, c, starti, a, &ai);
			if(tag == -1)
				break;
			else if(dbglex > 1 && tag != Comment)
				fprint(2, "lex: got token %T\n", &a[ai-1]);
		}
	}
	else {
		// plain text (non-html) tokens
		for(;;) {
			if(alen - ai < ToksChunk/32) {
				alen += ToksChunk;
				a = erealloc(a, alen*sizeof *a);
			}
			tag = getplaindata(ts, a, &ai);
			if(tag == -1)
				break;
			if(dbglex > 1)
				fprint(2, "lex: got token %T\n", &a[ai]);
		}
	}
	free(ts);
	if(dbglex)
		fprint(2, "lex: returning %d tokens\n", ai);
	*plen = ai;
	if(ai == 0){
		free(a);
		a = 0;
	}
	return a;
}

// For case where source isn't HTML.
// Just make data tokens, one per line (or partial line,
// at end of buffer), ignoring non-whitespace control
// characters and dumping \r's.
// If find non-empty token, fill in a[*pai], bump *pai, and return Data.
// Otherwise return -1;
static int
getplaindata(TokenSource* ts, Token* a, int* pai)
{
	Rune*	s;
	int	j;
	int	starti;
	int	c;
	Token*	tok;
	Rune	buf[BIGBUFSIZE];

	s = nil;
	j = 0;
	starti = ts->i;
	for(c = getchar(ts); c >= 0; c = getchar(ts)) {
		if(c < ' ') {
			if(isspace(c)) {
				if(c == '\r') {
					// ignore it unless no following '\n',
					// in which case treat it like '\n'
					c = getchar(ts);
					if(c != '\n') {
						if(c >= 0)
							ungetchar(ts, c);
						c = '\n';
					}
				}
			}
			else
				c = 0;
		}
		if(c != 0) {
			buf[j++] = c;
			if(j == nelem(buf)-1) {
				s = buftostr(s, buf, j);
				j = 0;
			}
		}
		if(c == '\n')
			break;
	}
	s = buftostr(s, buf, j);
	if(s == nil)
		return -1;
	tok = &a[(*pai)++];
	tok->tag = Data;
	tok->text = s;
	tok->attr = nil;
	tok->starti = starti;
	return Data;
}

// Return concatenation of s and buf[0:j]
static Rune*
buftostr(Rune* s, Rune* buf, int j)
{
	int i;

	if(s == nil)
		s = _Strndup(buf, j);
	else {
		i = _Strlen(s);
		s = realloc(s, ( i+j+1)*sizeof *s);
		memcpy(&s[i], buf, j*sizeof *s);
		s[i+j] = 0;
	}
	return s;
}

// Gather data up to next start-of-tag or end-of-buffer.
// Translate entity references (&amp;).
// Ignore non-whitespace control characters and get rid of \r's.
// If find non-empty token, fill in a[*pai], bump *pai, and return Data.
// Otherwise return -1;
static int
getdata(TokenSource* ts, int firstc, int starti, Token* a, int* pai)
{
	Rune*	s;
	int	j;
	int	c;
	Token*	tok;
	Rune	buf[SMALLBUFSIZE];

	s = nil;
	j = 0;
	for(c = firstc; c >= 0; c = getchar(ts)){
		if(c == '&') {
			c = ampersand(ts);
			if(c < 0)
				break;
		}
		else if(c < ' ') {
			if(isspace(c)) {
				if(c == '\r') {
					// ignore it unless no following '\n',
					// in which case treat it like '\n'
					c = getchar(ts);
					if(c != '\n') {
						if(c >= 0)
							ungetchar(ts, c);
						c = '\n';
					}
				}
			}
			else {
				if(warn)
					fprint(2, "warning: non-whitespace control character %d ignored\n", c);
				c = 0;
			}
		}
		else if(c == '<') {
			ungetchar(ts, c);
			break;
		}
		if(c != 0) {
			buf[j++] = c;
			if(j == nelem(buf)-1) {
				s = buftostr(s, buf, j);
				j = 0;
			}
		}
	}
	s = buftostr(s, buf, j);
	if(s == nil)
		return -1;
	tok = &a[(*pai)++];
	tok->tag = Data;
	tok->text = s;
	tok->attr = nil;
	tok->starti = starti;
	return Data;
}

// The rules for lexing scripts are different (ugh).
// Gather up everything until see an "</" tagnames[tok] ">"
static int
getscriptdata(TokenSource* ts, int firstc, int starti, Token* a, int* pai, int findtag)
{
	Rune*	s;
	int	j;
	int	tstarti;
	int	savei;
	int	c;
	int	tag;
	int	done;
	Token*	tok;
	Rune	buf[BIGBUFSIZE];

	s = nil;
	j = 0;
	tstarti = starti;
	c = firstc;
	done = 0;
	while(c >= 0) {
		if(c == '<') {
			// other browsers ignore stuff to end of line after <!
			savei = ts->i;
			c = getchar(ts);
			if(c == '!') {
				if(comment(ts) == -1)
					break;
				if(c == '\r')
					c = getchar(ts);
				if(c == '\n')
					c = getchar(ts);
			}
			else if(c >= 0) {
				backup(ts, savei);
				tag = gettag(ts, tstarti, a, pai);
				if(tag == -1)
					break;
				if(tag != Comment)
					(*pai)--;
				backup(ts, tstarti);
				if(tag == findtag + RBRA) {
					done = 1;
					break;
				}
				// here tag was not the one we were looking for, so take as regular data
				c = getchar(ts);
			}
		}
		if(c < 0)
			break;
		if(c != 0) {
			buf[j++] = c;
			if(j == nelem(buf)-1) {
				s = buftostr(s, buf, j);
				j = 0;
			}
		}
		tstarti = ts->i;
		c = getchar(ts);
	}
	if(done || ts->i == ts->edata) {
		s = buftostr(s, buf, j);
		tok = &a[(*pai)++];
		tok->tag = Data;
		tok->text = s;
		tok->attr = nil;
		tok->starti = starti;
		return Data;
	}
	free(s);
	backup(ts, starti);
	return -1;
}

// We've just seen a '<'.  Gather up stuff to closing '>' (if buffer
// ends before then, return -1).
// If it's a tag, look up the name, gather the attributes, and return
// the appropriate token.
// Else it's either just plain data or some kind of ignorable stuff:
// return Data or Comment as appropriate.
// If it's not a Comment, put it in a[*pai] and bump *pai.
static int
gettag(TokenSource* ts, int starti, Token* a, int* pai)
{
	int	rbra;
	int	ans;
	Attr*	al;
	int	nexti;
	int	c;
	int	ti;
	int	afnd;
	int	attid;
	int	quote;
	Rune*	val;
	int	nv;
	int	i;
	int	tag;
	Token*	tok;
	Rune	buf[BIGBUFSIZE];

	rbra = 0;
	nexti = ts->i;
	tok = &a[*pai];
	tok->tag = Notfound;
	tok->text = nil;
	tok->attr = nil;
	tok->starti = starti;
	c = getchar(ts);
	if(c == '/') {
		rbra = RBRA;
		c = getchar(ts);
	}
	if(c < 0)
		goto eob_done;
	if(c >= 256 || !isalpha(c)) {
		// not a tag
		if(c == '!') {
			ans = comment(ts);
			if(ans != -1)
				return ans;
			goto eob_done;
		}
		else {
			backup(ts, nexti);
			tok->tag = Data;
			tok->text = _Strdup((Rune*)L"<");
			(*pai)++;
			return Data;
		}
	}
	// c starts a tagname
	buf[0] = c;
	i = 1;
	while(1) {
		c = getchar(ts);
		if(c < 0)
			goto eob_done;
		if(!ISNAMCHAR(c))
			break;
		// if name is bigger than buf it won't be found anyway...
		if(i < BIGBUFSIZE)
			buf[i++] = c;
	}
	if(_lookup(tagtable, Numtags, buf, i, &tag))
		tok->tag = tag + rbra;
	else
		tok->text = _Strndup(buf, i);	// for warning print, in build
	// attribute gathering loop
	al = nil;
	while(1) {
		// look for "ws name" or "ws name ws = ws val"  (ws=whitespace)
		// skip whitespace
attrloop_continue:
		while(c < 256 && isspace(c)) {
			c = getchar(ts);
			if(c < 0)
				goto eob_done;
		}
		if(c == '>')
			goto attrloop_done;
		if(c == '<') {
			if(warn)
				fprint(2, "warning: unclosed tag\n");
			ungetchar(ts, c);
			goto attrloop_done;
		}
		if(c >= 256 || !isalpha(c)) {
			if(warn)
				fprint(2, "warning: expected attribute name\n");
			// skipt to next attribute name
			while(1) {
				c = getchar(ts);
				if(c < 0)
					goto eob_done;
				if(c < 256 && isalpha(c))
					goto attrloop_continue;
				if(c == '<') {
					if(warn)
						fprint(2, "warning: unclosed tag\n");
					ungetchar(ts, 60);
					goto attrloop_done;
				}
				if(c == '>')
					goto attrloop_done;
			}
		}
		// gather attribute name
		buf[0] = c;
		i = 1;
		while(1) {
			c = getchar(ts);
			if(c < 0)
				goto eob_done;
			if(!ISNAMCHAR(c))
				break;
			if(i < BIGBUFSIZE-1)
				buf[i++] = c;
		}
		afnd = _lookup(attrtable, Numattrs, buf, i, &attid);
		if(warn && !afnd) {
			buf[i] = 0;
			fprint(2, "warning: unknown attribute name %S\n", buf);
		}
		// skip whitespace
		while(c < 256 && isspace(c)) {
			c = getchar(ts);
			if(c < 0)
				goto eob_done;
		}
		if(c != '=') {
			if(afnd)
				al = newattr(attid, nil, al);
			goto attrloop_continue;
		}
		//# c is '=' here;  skip whitespace
		while(1) {
			c = getchar(ts);
			if(c < 0)
				goto eob_done;
			if(c >= 256 || !isspace(c))
				break;
		}
		quote = 0;
		if(c == '\'' || c == '"') {
			quote = c;
			c = getchar(ts);
			if(c < 0)
				goto eob_done;
		}
		val = nil;
		nv = 0;
		while(1) {
valloop_continue:
			if(c < 0)
				goto eob_done;
			if(c == '>') {
				if(quote) {
					// c might be part of string (though not good style)
					// but if line ends before close quote, assume
					// there was an unmatched quote
					ti = ts->i;
					while(1) {
						c = getchar(ts);
						if(c < 0)
							goto eob_done;
						if(c == quote) {
							backup(ts, ti);
							buf[nv++] = '>';
							if(nv == BIGBUFSIZE-1) {
								val = buftostr(val, buf, nv);
								nv = 0;
							}
							c = getchar(ts);
							goto valloop_continue;
						}
						if(c == '\n') {
							if(warn)
								fprint(2, "warning: apparent unmatched quote\n");
							backup(ts, ti);
							c = '>';
							goto valloop_done;
						}
					}
				}
				else
					goto valloop_done;
			}
			if(quote) {
				if(c == quote) {
					c = getchar(ts);
					if(c < 0)
						goto eob_done;
					goto valloop_done;
				}
				if(c == '\r') {
					c = getchar(ts);
					goto valloop_continue;
				}
				if(c == '\t' || c == '\n')
					c = ' ';
			}
			else {
				if(c < 256 && isspace(c))
					goto valloop_done;
			}
			if(c == '&') {
				c = ampersand(ts);
				if(c == -1)
					goto eob_done;
			}
			buf[nv++] = c;
			if(nv == BIGBUFSIZE-1) {
				val = buftostr(val, buf, nv);
				nv = 0;
			}
			c = getchar(ts);
		}
valloop_done:
		if(afnd) {
			val = buftostr(val, buf, nv);
			al = newattr(attid, val, al);
		}
	}

attrloop_done:
	tok->attr = al;
	(*pai)++;
	return tok->tag;

eob_done:
	if(warn)
		fprint(2, "warning: incomplete tag at end of page\n");
	backup(ts, nexti);
	tok->tag = Data;
	tok->text = _Strdup((Rune*)L"<");
	return Data;
}

// We've just read a '<!' at position starti,
// so this may be a comment or other ignored section, or it may
// be just a literal string if there is no close before end of file
// (other browsers do that).
// The accepted practice seems to be (note: contrary to SGML spec!):
// If see <!--, look for --> to close, or if none, > to close.
// If see <!(not --), look for > to close.
// If no close before end of file, leave original characters in as literal data.
//
// If we see ignorable stuff, return Comment.
// Else return nil (caller should back up and try again when more data arrives,
// unless at end of file, in which case caller should just make '<' a data token).
static int
comment(TokenSource* ts)
{
	int	nexti;
	int	havecomment;
	int	c;

	nexti = ts->i;
	havecomment = 0;
	c = getchar(ts);
	if(c == '-') {
		c = getchar(ts);
		if(c == '-') {
			if(findstr(ts, (Rune*)L"-->"))
				havecomment = 1;
			else
				backup(ts, nexti);
		}
	}
	if(!havecomment) {
		if(c == '>')
			havecomment = 1;
		else if(c >= 0) {
			if(findstr(ts, (Rune*)L">"))
				havecomment = 1;
		}
	}
	if(havecomment)
		return Comment;
	return -1;
}

// Look for string s in token source.
// If found, return 1, with buffer at next char after s,
// else return 0 (caller should back up).
static int
findstr(TokenSource* ts, Rune* s)
{
	int	c0;
	int	n;
	int	nexti;
	int	i;
	int	c;

	c0 = s[0];
	n = runestrlen(s);
	while(1) {
		c = getchar(ts);
		if(c < 0)
			break;
		if(c == c0) {
			if(n == 1)
				return 1;
			nexti = ts->i;
			for(i = 1; i < n; i++) {
				c = getchar(ts);
				if(c < 0)
					goto mainloop_done;
				if(c != s[i])
					break;
			}
			if(i == n)
				return 1;
			backup(ts, nexti);
		}
	}
mainloop_done:
	return 0;
}

// We've just read an '&'; look for an entity reference
// name, and if found, return translated char.
// if there is a complete entity name but it isn't known,
// back up to just past the '&' and return '&'.
// If the entity can't be completed in the current buffer, back up
// to the '&' and return -1.
static int
ampersand(TokenSource* ts)
{
	int	savei;
	int	c;
	int	fnd;
	int	ans;
	int	v;
	int	k;
	Rune	buf[25];

	savei = ts->i;
	c = getchar(ts);
	fnd = 0;
	ans = -1;
	if(c == '#') {
		c = getchar(ts);
		v = 0;
		if(c == 'X' || c == 'x')
			for(c = getchar(ts); c < 256; c = getchar(ts))
				if(c >= '0' && c <= '9')
					v = v*16+c-'0';
				else if(c >= 'A' && c<= 'F')
					v = v*16+c-'A'+10;
				else if(c >= 'a' && c <= 'f')
					v = v*16+c-'a'+10;
				else
					break;
		else
			while(c >= 0) {
				if(!(c < 256 && isdigit(c)))
					break;
				v = v*10 + c - 48;
				c = getchar(ts);
			}
		if(c >= 0) {
			if(!(c == ';' || c == '\n' || c == '\r'))
				ungetchar(ts, c);
			c = v;
			if(c == 160)
				c = 160;
			if(c >= Winstart && c <= Winend) {
				c = winchars[c - Winstart];
			}
			ans = c;
			fnd = 1;
		}
	}
	else if(c < 256 && isalpha(c)) {
		buf[0] = c;
		k = 1;
		while(1) {
			c = getchar(ts);
			if(c < 0)
				break;
			if(c < 256 && (isalpha(c) || isdigit(c))) {
				if(k < nelem(buf)-1)
					buf[k++] = c;
			}
			else {
				if(!(c == ';' || c == '\n' || c == '\r'))
					ungetchar(ts, c);
				break;
			}
		}
		if(c >= 256 || c != '=' && !(isalpha(c) || isdigit(c)))
			fnd = _lookup(chartab, NCHARTAB, buf, k, &ans);
	}
	if(!fnd) {
		backup(ts, savei);
		ans = '&';
	}
	return ans;
}

// Get next char, obeying ts.chset.
// Returns -1 if no complete character left before current end of data.
static int
getchar(TokenSource* ts)
{
	uint8_t*	buf;
	int	c;
	int	n;
	int	ok;
	Rune	r;

	if(ts->i >= ts->edata)
		return -1;
	buf = ts->data;
	c = buf[ts->i];
	switch(ts->chset) {
	case ISO_8859_1:
		if(c >= Winstart && c <= Winend)
			c = winchars[c - Winstart];
		ts->i++;
		break;
	case US_Ascii:
		if(c > 127) {
			if(warn)
				fprint(2, "non-ascii char (%x) when US-ASCII specified\n", c);
		}
		ts->i++;
		break;
	case UTF_8:
		ok = fullrune((char*)(buf+ts->i), ts->edata-ts->i);
		n = chartorune(&r, (char*)(buf+ts->i));
		if(ok) {
			if(warn && c == 0x80)
				fprint(2, "warning: invalid utf-8 sequence (starts with %x)\n", ts->data[ts->i]);
			ts->i += n;
			c = r;
		}
		else {
			// not enough bytes in buf to complete utf-8 char
			ts->i = ts->edata;	// mark "all used"
			c = -1;
		}
		break;
	case Unicode:
		if(ts->i < ts->edata - 1) {
			//standards say most-significant byte first
			c = (c << 8)|(buf[ts->i + 1]);
			ts->i += 2;
		}
		else {
			ts->i = ts->edata;	// mark "all used"
			c = -1;
		}
		break;
	default:
		return -1;
	}
	return c;
}

// Assuming c was the last character returned by getchar, set
// things up so that next getchar will get that same character
// followed by the current 'next character', etc.
static void
ungetchar(TokenSource* ts, int c)
{
	int	n;
	Rune	r;
	char	a[UTFmax];

	n = 1;
	switch(ts->chset) {
	case UTF_8:
		if(c >= 128) {
			r = c;
			n = runetochar(a, &r);
		}
		break;
	case Unicode:
		n = 2;
		break;
	}
	ts->i -= n;
}

// Restore ts so that it is at the state where the index was savei.
static void
backup(TokenSource* ts, int savei)
{
	if(dbglex)
		fprint(2, "lex: backup; i=%d, savei=%d\n", ts->i, savei);
	ts->i = savei;
}


// Look for value associated with attribute attid in token t.
// If there is one, return 1 and put the value in *pans,
// else return 0.
// If xfer is true, transfer ownership of the string to the caller
// (nil it out here); otherwise, caller must duplicate the answer
// if it needs to save it.
// OK to have pans==0, in which case this is just looking
// to see if token is present.
int
_tokaval(Token* t, int attid, Rune** pans, int xfer)
{
	Attr*	attr;

	attr = t->attr;
	while(attr != nil) {
		if(attr->attid == attid) {
			if(pans != nil)
				*pans = attr->value;
			if(xfer)
				attr->value = nil;
			return 1;
		}
		attr = attr->next;
	}
	if(pans != nil)
		*pans = nil;
	return 0;
}

static int
Tconv(Fmt *f)
{
	Token*	t;
	int	i;
	int	tag;
	char*	srbra;
	Rune*	aname;
	Rune*	tname;
	Attr*	a;
	char	buf[BIGBUFSIZE];

	t = va_arg(f->args, Token*);
	if(t == nil)
		sprint(buf, "<null>");
	else {
		i = 0;
		if(dbglex > 1)
			i = snprint(buf, sizeof(buf), "[%d]", t->starti);
		tag = t->tag;
		if(tag == Data) {
			i += snprint(buf+i, sizeof(buf)-i-1, "'%S'", t->text);
		}
		else {
			srbra = "";
			if(tag >= RBRA) {
				tag -= RBRA;
				srbra = "/";
			}
			tname = tagnames[tag];
			if(tag == Notfound)
				tname = (Rune*)L"?";
			i += snprint(buf+i, sizeof(buf)-i-1, "<%s%S", srbra, tname);
			for(a = t->attr; a != nil; a = a->next) {
				aname = attrnames[a->attid];
				i += snprint(buf+i, sizeof(buf)-i-1, " %S", aname);
				if(a->value != nil)
					i += snprint(buf+i, sizeof(buf)-i-1, "=%S", a->value);
			}
			i += snprint(buf+i, sizeof(buf)-i-1, ">");
		}
		buf[i] = 0;
	}
	return fmtstrcpy(f, buf);
}

// Attrs own their constituent strings, but build may eventually
// transfer some values to its items and nil them out in the Attr.
static Attr*
newattr(int attid, Rune* value, Attr* link)
{
	Attr* ans;

	ans = (Attr*)emalloc(sizeof(Attr));
	ans->attid = attid;
	ans->value = value;
	ans->next = link;
	return ans;
}

// Free list of Attrs linked through next field
static void
freeattrs(Attr* ahead)
{
	Attr* a;
	Attr* nexta;

	a = ahead;
	while(a != nil) {
		nexta = a->next;
		free(a->value);
		free(a);
		a = nexta;
	}
}

// Free array of Tokens.
// Allocated space might have room for more than n tokens,
// but only n of them are initialized.
// If caller has transferred ownership of constitutent strings
// or attributes, it must have nil'd out the pointers in the Tokens.
void
_freetokens(Token* tarray, int n)
{
	int i;
	Token* t;

	if(tarray == nil)
		return;
	for(i = 0; i < n; i++) {
		t = &tarray[i];
		free(t->text);
		freeattrs(t->attr);
	}
	free(tarray);
}

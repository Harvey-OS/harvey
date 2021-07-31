/*
 * Parameters
 */
#define	NSTACK	100	/* html grammar is not recursive, so 30 or so should do */
#define	NHBUF	8192	/* Input buffer size */
#define	NPEEKC	3	/* Maximum lookahead */
#define	NTOKEN	1024	/* Maximum token length */
#define	NATTR	512	/* Maximum number of attributes of a tag */
typedef struct Pair Pair;
typedef struct Tag Tag;
typedef struct Stack Stack;
typedef struct Hglob Hglob;
typedef struct Form Form;
struct Pair{
	char *name;
	char *value;
};
struct Tag{
	char *name;
	int action;
};
struct Stack{
	int tag;		/* html tag being processed */
	int pre;		/* in preformatted text? */
	int font;		/* typeface */
	int size;		/* point size of text */
	int margin;		/* left margin position */
	int indent;		/* extra indent at paragraph start */
	int number;		/* paragraph number */
	char *image;		/* arg of <img> */
	char *link;		/* arg of <a href=...> */
	char *name;		/* arg of <a name=...> */
	int ismap;		/* flag of <img> */
};
/*
 * Globals -- these are packed up into a struct that gets passed around
 * so that multiple parsers can run concurrently
 */
struct Hglob{
	char *name;		/* input file name */
	int hfd;		/* input file descriptor */
	char hbuf[NHBUF];	/* input buffer */
	char *hbufp;		/* next character in buffer */
	char *ehbuf;		/* end of good characters in buffer */
	int heof;		/* end of file flag */
	int peekc[NPEEKC];	/* characters to re-read */
	int npeekc;		/* # of characters to re-read */
	char token[NTOKEN];	/* if token type is TEXT */
	Pair attr[NATTR];	/* tag attribute/value pairs */
	int nsp;		/* # of white-space characters before TEXT token */
	int spacc;		/* place to accumulate more spaces */
				/* if negative, won't accumulate! */
	int tag;		/* if token type is TAG or END */
	Stack stack[NSTACK];	/* parse stack */
	Stack *state;		/* parse stack pointer */
	int lineno;		/* input line number */
	int linebrk;		/* flag set if we require a line-break in output */
	int para;		/* flag set if we need an indent at the break */
	char *text;		/* text buffer */
	char *etext;		/* end of text buffer */
	char *tp;		/* pointer in text buffer */
	Form *form;		/* data for form under construction */
	Www *dst;		/* where the text goes */
};
/*
 * Token types
 */
#define	TAG	1
#define	ENDTAG	2
#define	TEXT	3
/*
 * Magic characters corresponding to
 *	literal < followed by / ! or alpha,
 *	literal > and
 *	end of file
 */
#define	STAG	65536
#define	ETAG	65537
#define	EOF	(-1)
/*
 * fonts
 */
#define	ROMAN	0
#define	ITALIC	1
#define	BOLD	2
#define	CWIDTH	3
/*
 * font sizes
 */
#define	SMALL	0
#define	NORMAL	1
#define	LARGE	2
#define	ENORMOUS 3
/*
 * Token names for the html parser.
 * Tag_end corresponds to </end> tags.
 * Tag_text tags text not in a tag.
 * Those two must follow the others.
 */
enum{
	Tag_comment=0,
	Tag_a=1,
	Tag_address=2,
	Tag_b=3,
	Tag_base=4,
	Tag_blockquot=5,
	Tag_body=6,
	Tag_br=7,
	Tag_cite=8,
	Tag_code=9,
	Tag_dd=10,
	Tag_dfn=11,
	Tag_dir=12,
	Tag_dl=13,
	Tag_dt=14,
	Tag_em=15,
	Tag_form=16,
	Tag_h1=17,
	Tag_h2=18,
	Tag_h3=19,
	Tag_h4=20,
	Tag_h5=21,
	Tag_h6=22,
	Tag_head=23,
	Tag_hr=24,
	Tag_html=25,
	Tag_i=26,
	Tag_img=27,
	Tag_input=28,
	Tag_isindex=29,
	Tag_kbd=30,
	Tag_key=31,
	Tag_li=32,
	Tag_link=33,
	Tag_listing=34,
	Tag_menu=35,
	Tag_meta=36,
	Tag_nextid=37,
	Tag_ol=38,
	Tag_option=39,
	Tag_p=40,
	Tag_plaintext=41,
	Tag_pre=42,
	Tag_samp=43,
	Tag_select=44,
	Tag_strong=45,
	Tag_textarea=46,
	Tag_title=47,
	Tag_tt=48,
	Tag_u=49,
	Tag_ul=50,
	Tag_var=51,
	Tag_xmp=52,
	Tag_end=53,		/* also used to indicate unrecognized start tag */
	Tag_text=54,
	NTAG=Tag_end,
	END=1,			/* tag must have a matching end tag */
	NOEND=2,		/* tag must not have a matching end tag */
	OPTEND=3,		/* tag may have a matching end tag */
	ERR=4,			/* tag must not occur */
};
Tag tag[];
Pair pl_entity[];
void rdform(Hglob *);
void endform(Hglob *);
char *pl_getattr(Pair *, char *);
int pl_hasattr(Pair *, char *);
void pl_htmloutput(Hglob *, int, char *, Field *);

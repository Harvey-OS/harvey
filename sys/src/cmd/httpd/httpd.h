enum
{
	BufSize	= 8192,
	MaxWord	= 1024,

	/*
	 * tokens
	 */
	Word	= 1,

	/*
	 * error messages
	 */
	Internal	= 0,
	TempFail,
	Unimp,
	BadReq,
	BadSearch,
	NotFound,
	Unauth,
	Syntax,
	NoSearch,
	OnlySearch,
	UnkVers,
	BadCont,
	OK,
};

aggr Keyword
{
	byte	*name;
	void	(*parse)(byte*);
};

aggr Content
{
	Content	*next;
	byte	*generic;
	byte	*specific;
	float	q;		/* desirability of this kind of file */
	int	mxb;		/* max bytes until worthless */
};

aggr Query			/* list of tag=val pairs from a search string */
{
	byte	*tag;
	byte	*val;
	Query	*next;
};

extern	byte*	HTTPLOG;
extern	Biobuf	bout;
extern	byte*	client;
extern	uint	modtime;
extern	byte*	mydomain;
extern	byte*	mysysname;
extern	byte*	namespace;
extern	Content*	okencode;
extern	Content*	oklang;
extern	Content*	oktype;
extern	byte*	remotesys;
extern	byte*	version;
extern	byte	xferbuf[BufSize];	/* buffer for making up or transferring data */

void		anonymous(byte*);
int		checkcontent(Content*, Content*, byte*, int);
void		checkfail(byte*, byte*);
(Content*,Content*)	classify(byte*);
void		contentinit(void);
uint		date2sec(byte*);
int		dateconv(Printspec*);
void		fail(int, ...);
void		freequery(Query*);
int		gmtm2sec(Tm);
int		httpconv(Printspec*);
void		httpheaders(byte*);
(byte*, byte*, byte*, byte*)	init(int, byte**);
int		lex();
void		lexhead();
void		lexinit();
void		logcontent(byte*, Content*);
void		logit(byte*, ...);
Content*		mkcontent(byte*, byte*, Content*);
Query*		mkquery(byte*, byte*, Query*);
void		notfound(int, byte*);
void		notmodified(void);
void		okheaders();
void		parsejump(Keyword*, byte*);
Query*		parsequery(byte*);
void		parsereq();
void		send(byte*, byte*, byte*, byte*);
byte*		strsave(byte*);
byte		*urlunesc(byte*);

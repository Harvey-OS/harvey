#include <alef.h>
#include <bio.h>
#include <auth.h>
#include "httpd.h"

aggr Error
{
	byte	*num;
	byte	*concise;
	byte	*verbose;
};

Error errormsg[] =
{
	[Internal]	{"500 Internal Error", "Internal Error",
		"This server could not process your request due to an interal error."},
	[TempFail]	{"500 Internal Error", "Temporary Failure",
		"The object %s is currently inaccessible.<p>Please try again later."},
	[Unimp]	{"501 Not implemented", "Command not implemented",
		"This server does not implement the %s command."},
	[UnkVers]	{"501 Not Implemented", "Unknown http version",
		"This server does not know how to respond to http version %s."},
	[BadCont]	{"501 Not Implemented", "Impossible format",
		"This server cannot produce %s in any of the formats your client accepts."},
	[BadReq]	{"400 Bad Request", "Strange Request",
		"Your client sent a query that this server could not understand."},
	[Syntax]	{"400 Bad Request", "Garbled Syntax",
		"Your client sent a query with incoherent syntax."},
	[BadSearch]{"400 Bad Request", "Inapplicable Search",
		"Your client sent a search that cannot be applied to %s."},
	[NotFound]{"404 Not Found", "Object not found",
		"The object %s does not exist on this server."},
	[NoSearch]	{"403 Forbidden", "Search not supported",
		"The object %s does not support the search command."},
	[OnlySearch]{"403 Forbidden", "Searching Only",
		"The object %s only supports the searching methods."},
	[Unauth]	{"401 Unauthorized", "Unauthorized",
		"You are not authorized to see the object %s."},
	[OK]	{"200 OK", "everything is fine"},
};

void		mimeaccept(byte*);
void		mimeacceptenc(byte*);
void		mimeacceptlang(byte*);
void		mimeagent(byte*);
void		mimefrom(byte*);
void		mimeignore(byte*);
void		mimemodified(byte*);
void		mimeunknown(byte*);

Keyword	mime[] =
{
	{"from",			mimefrom},
	{"if-modified-since",		mimemodified},
	{"accept",			mimeaccept},
	{"accept-encoding",		mimeacceptenc},
	{"accept-language",		mimeacceptlang},
	{"user-agent",		mimeagent},
	{"referer",			mimeignore},
	{"authorization",		mimeignore},
	{"forwarded",		mimeignore},
	{"chargeto",		mimeignore},
	{"x-serial-number",		mimeignore},
	{"proxy-agent",		mimeignore},
	{"x-afs-tokens",		mimeignore},
	{nil,			mimeunknown},
};


byte*	HTTPLOG = "httpd";
Biobuf	bout;
byte*	client = "";
uint	modtime;
byte*	mydomain;
byte*	mysysname;
byte*	namespace;
Content*	okencode = nil;
Content*	oklang = nil;
Content*	oktype = nil;
byte*	remotesys;
byte*	version = "HTTP/1.0";
byte	xferbuf[BufSize];

int	ALEFstack = 64*1024;

intern	byte	wordval[MaxWord];
intern	byte	makeup[BufSize];
intern	int	tok;
intern	int	eof;
intern	Biobuf	bin;

intern	int	getc();
intern	void	ungetc();
intern	int	wordcr();
intern	int	wordnl();
intern	void	word(byte*);
intern	int	lex1();

void
anonymous(byte *namespace)
{
	newns("none", namespace);
	chdir("/");
}

void
httpheaders(byte *vers)
{
	if(vers[0] == 0)
		return;

	lexinit();
	tok = '\n';
	alarm(15*60*1000);
	while(lex() != '\n'){
		if(tok == Word && lex() == ':')
			parsejump(mime, strsave(wordval));
		while(tok != '\n')
			lex();
	}
	alarm(0);
}

Content *
mimeok(byte *name, int multipart, Content *head)
{
	byte *generic, *specific, *s;
	float v;

	while(lex() != Word)
		if(tok != ',')
			return head;

	generic = strsave(wordval);
	lex();
	if(tok == '/' || multipart){
		if(tok != '/')
			return head;
		if(lex() != Word)
			return head;
		specific = strsave(wordval);
		lex();
	}else
		specific = "*";
	head = mkcontent(generic, specific, head);

	for(;;){
		switch(tok){
		case ';':
			if(lex() == Word){
				s = strsave(wordval);
				if(lex() != '=' || lex() != Word)
					return head;
				v = strtof(wordval, nil);
				if(strcmp(s, "q") == 0)
					head->q = v;
				else if(strcmp(s, "mxb") == 0)
					head->mxb = v;
				else
					logit("unknown %s param: %s %s",
						name, s, wordval);
			}
			break;
		case ',':
			return  mimeok(name, multipart, head);
		default:
			return head;
		}
		lex();
	}
	return head;
}

void
mimeaccept(byte *name)
{
	oktype = mimeok(name, 1, oktype);
}

void
mimeacceptenc(byte *name)
{
	okencode = mimeok(name, 0, okencode);
}

void
mimeacceptlang(byte *name)
{
	oklang = mimeok(name, 0, oklang);
}

void
mimemodified(byte *name)
{
	lexhead();
	modtime = date2sec(wordval);
	if(modtime == 0)
		logit("%s: %s", name, wordval);
}

void
mimeagent(byte *)
{
	lexhead();
	client = strsave(wordval);
}

void
mimefrom(byte *)
{
	lexhead();
}

void
mimeignore(byte *)
{
	lexhead();
}

void
mimeunknown(byte *name)
{
	lexhead();
	if(client[0])
		logit("agent %s: ignoring header %s: %s ", client, name, wordval);
	else
		logit("ignoring header %s: %s", name, wordval);
}

void
parsejump(Keyword *keys, byte *k)
{
	for(; keys->name; keys++)
		if(strcmp(keys->name, k) == 0)
			break;
	(*keys->parse)(k);
}

void
lexinit()
{
	bin.init(0, OREAD);
}

int
lex()
{
	tok = lex1();
	return tok;
}

/*
 * rfc 822/rfc 1521 lexical analyzer
 */
intern int
lex1()
{
	int level, c;

	if(eof)
		return '\n';

top:
	c = getc();
	switch(c){
	case '(':
		level = 1;
		while((c = getc()) != Beof){
			if(c == '\\'){
				c = getc();
				if(c == Beof)
					return '\n';
				continue;
			}
			if(c == '(')
				level++;
			else if(c == ')' && --level == 0)
				break;
			else if(c == '\n'){
				c = getc();
				if(c == Beof)
					return '\n';
				if(c == ')' && --level == 0)
					break;
				if(c != ' ' && c != '\t'){
					ungetc();
					return '\n';
				}
			}
		}
		goto top;

	case ' ': case '\t':
		goto top;

	case '\r':
		c = getc();
		if(c != '\n'){
			ungetc();
			goto top;
		}

	case '\n':
		if(tok == '\n'){
			eof = 1;
			return '\n';
		}
		c = getc();
		if(c == Beof)
			return '\n';
		if(c != ' ' && c != '\t'){
			ungetc();
			return '\n';
		}
		goto top;

	case ')':
	case '<': case '>':
	case '[': case ']':
	case '@': case '/':
	case ',': case ';': case ':': case '?': case '=':
		return c;

	case '"':
		word("\"");
		getc();		/* skip the closing quote */
		return Word;

	default:
		ungetc();
		word("\"()<>@,;:/[]?=\r\n \t");
		return Word;
	}
	goto top;
	return 0;	
}

/*
 * return the rest of an rfc 822, not including \r or \n
 * do not map to lower case
 */
void
lexhead()
{
	int c, n;

	n = 0;
	while((c = getc()) != Beof){
		if(c == '\r')
			c = wordcr();
		else if(c == '\n')
			c = wordnl();
		if(c == '\n')
			break;
		if(c == '\\'){
			c = getc();
			if(c == Beof)
				break;
		}

		if(n < MaxWord-1)
			wordval[n++] = c;
	}
	tok = '\n';
	wordval[n] = 0;
}

intern void
word(byte *stop)
{
	int c, n;

	n = 0;
	while((c = getc()) != Beof){
		if(c == '\r')
			c = wordcr();
		else if(c == '\n')
			c = wordnl();
		if(c == '\\'){
			c = getc();
			if(c == Beof)
				break;
		}else if(strchr(stop, c)){
			ungetc();
			wordval[n] = 0;
			return;
		}

		if(c >= 'A' && c <= 'Z')
			c += 'a' - 'A';
		if(n < MaxWord-1)
			wordval[n++] = c;
	}
	wordval[n] = 0;
}

intern int
wordcr()
{
	int c;

	c = getc();
	if(c == '\n')
		return wordnl();
	ungetc();
	return ' ';
}

intern int
wordnl()
{
	int c;

	c = getc();
	if(c == ' ' || c == '\t')
		return c;
	ungetc();

	return '\n';
}

intern int
getc(void)
{
	int c;

	c = bin.getc();
	if(c == Beof){
		eof = 1;
		return c;
	}
	return c & 0x7f;
}

intern void
ungetc(void)
{
	bin.ungetc();
}

byte *
urlunesc(byte *s)
{
	byte *t, *v;
	int c, n;

	v = malloc(strlen(s) + 1);
	for(t = v; c = *s; s++){
		if(c == '%'){
			n = s[1];
			if(n >= '0' && n <= '9')
				n = n - '0';
			else if(n >= 'A' && n <= 'F')
				n = n - 'A' + 10;
			else if(n >= 'a' && n <= 'f')
				n = n - 'a' + 10;
			else
				break;
			c = n;
			n = s[2];
			if(n >= '0' && n <= '9')
				n = n - '0';
			else if(n >= 'A' && n <= 'F')
				n = n - 'A' + 10;
			else if(n >= 'a' && n <= 'f')
				n = n - 'a' + 10;
			else
				break;
			s += 2;
			c = c * 16 + n;
		}
		*t++ = c;
	}
	*t = 0;
	return v;
}

/*
 * write a failure message to the net and exit
 */
void
fail(int reason, ...)
{
	int n;

	doprint(makeup, makeup+BufSize, errormsg[reason].verbose, ...);
	snprint(xferbuf, BufSize, "<head><title>%s</title></head>\n<body><h1>%s</h1>\n%s</body>\n",
		errormsg[reason].concise, errormsg[reason].concise, makeup);
	n = strlen(xferbuf);
	bout.print("%s %s\r\n", version, errormsg[reason].num);
	bout.print("Date: %D\r\n", time());
	bout.print("Server: Plan9\r\n");
	bout.print("MIME-version: 1.0\r\n");
	bout.print("Content-Type: text/html\r\n");
	bout.print("Content-Length: %d\r\n", n);
	bout.print("\r\n");
	bout.write(xferbuf, n);
	logit("failing: %s", makeup);
	exits("failed");
}

/*
 * internal alef check error
 * log it and quit
 */
void
checkfail(byte *file, byte *message)
{
	int n;

	snprint(xferbuf, BufSize, "<head><title>%s</title></head>\n<body><h1>%s</h1>\n%s</body>\n",
		errormsg[Internal].concise, errormsg[Internal].concise, errormsg[Internal].verbose);
	n = strlen(xferbuf);
	bout.print("%s %s\r\n", version, errormsg[Internal].num);
	bout.print("Date: %D\r\n", time());
	bout.print("Server: Plan9\r\n");
	bout.print("MIME-version: 1.0\r\n");
	bout.print("Content-Type: text/html\r\n");
	bout.print("Content-Length: %d\r\n", n);
	bout.print("\r\n");
	bout.write(xferbuf, n);
	logit("failing: check: %s: %s", file, message);
	exits("failed");
}

/*
 * write successful header
 */
void
okheaders()
{
	bout.print("%s 200 OK\r\n", version);
	bout.print("Server: Plan9\r\n");
	bout.print("MIME-version: 1.0\r\n");
}

void
notmodified()
{
	bout.print("%s 304 Not Modified\r\n", version);
	bout.print("Server: Plan9\r\n");
	bout.print("MIME-version: 1.0\r\n");
	exits(nil);
}

void
logit(byte *fmt, ...)
{
	byte buf[4096];

	doprint(buf, buf+sizeof(buf), fmt, ...);
	syslog(0, HTTPLOG, "%s %s", remotesys, buf);
}

int
httpconv(Printspec *p)
{
	byte buf[MaxWord*2], *s, *t, *e;
	int c;

	s = *(byte**)p->o;
	for(t = buf; t < buf +sizeof(buf)-8; ){
		c = *s++;
		if(c == 0)
			break;
		switch(c){
		case '"':	e = "&quot;";	break;
		case '&':	e = "&amp;";	break;
		case '<':	e = "&lt;";		break;
		case '>':	e = "&gt;";	break;
		default:	*t++ = c;		continue;
		}
		while(*t = *e++)
			t++;
	}
	*t = 0;
	strconv(p, buf);
	return sizeof(byte*);
}

byte*
strsave(byte *s)
{
	byte *t;
	int n;

	n = strlen(s);
	t = malloc(n+1);
	strcpy(t, s);
	return t;
}

/*
 * To do:
 *	<!doctype> not implemented
 *	forms
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "mothra.h"
#include "html.h"
typedef struct Fontdata Fontdata;
struct Fontdata{
	char *name;
	Font *font;
	int space;
}fontlist[4][4]={
	"lucidasans/latin1.6", 0, 0,
	"lucidasans/latin1.7", 0, 0,
	"lucidasans/latin1.10", 0, 0,
	"lucidasans/latin1.13", 0, 0,

	"lucidasans/italiclatin1.6", 0, 0,
	"lucidasans/italiclatin1.7", 0, 0,
	"lucidasans/italiclatin1.10", 0, 0,
	"lucidasans/italiclatin1.13", 0, 0,

	"lucidasans/boldlatin1.6", 0, 0,
	"lucidasans/boldlatin1.7", 0, 0,
	"lucidasans/boldlatin1.10", 0, 0,
	"lucidasans/boldlatin1.13", 0, 0,

	"lucidasans/typelatin1.6", 0, 0,
	"lucidasans/typelatin1.7", 0, 0,
	"pelm/ascii.12", 0, 0,
	"pelm/ascii.16", 0, 0,
};
Fontdata *pl_whichfont(int f, int s){
	char name[100];
	if(fontlist[f][s].font==0){
		sprint(name, "/lib/font/bit/%s.font",
			fontlist[f][s].name);
		fontlist[f][s].font=rdfontfile(name, screen.ldepth);
		if(fontlist[f][s].font==0) fontlist[f][s].font=font;
		fontlist[f][s].space=strwidth(fontlist[f][s].font, "0");
	}
	return &fontlist[f][s];
	
}
void getfonts(void){
	int f, s;
	for(f=0;f!=4;f++)
		for(s=0;s!=4;s++)
			pl_whichfont(f, s);
}
void pl_pushstate(Hglob *g, int t){
	++g->state;
	if(g->state==&g->stack[NSTACK]){
		htmlerror(g->name, g->lineno, "stack overflow at <%s>", tag[t].name);
		--g->state;
	}
	g->state[0]=g->state[-1];
	g->state->tag=t;
}
void pl_linespace(Hglob *g){
	plrtstr(&g->dst->text, 1000000, 1000000, font, "", 0, 0);
	g->para=0;
	g->linebrk=0;
}
void pl_htmloutput(Hglob *g, int nsp, char *s, Field *field){
	Fontdata *f;
	int space, indent;
	Action *ap;
	if(g->state->tag==Tag_title
/*	|| g->state->tag==Tag_textarea */
	|| g->state->tag==Tag_select){
		if(s){
			if(g->tp!=g->text && g->tp!=g->etext && g->tp[-1]!=' ')
				*g->tp++=' ';
			while(g->tp!=g->etext && *s) *g->tp++=*s++;
			if(g->state->tag==Tag_title) g->dst->changed=1;
		}
		return;
	}
	f=pl_whichfont(g->state->font, g->state->size);
	space=f->space;
	indent=g->state->margin;
	if(g->para){
		space=1000000;
		indent+=g->state->indent;
	}
	else if(g->linebrk)
		space=1000000;
	else if(nsp<=0)
		space=0;
	if(g->state->image==0 && g->state->link==0 && g->state->name==0 && field==0)
		ap=0;
	else{
		ap=malloc(sizeof(Action));
		if(ap!=0){
			ap->image=g->state->image;
			ap->link=g->state->link;
			ap->name=g->state->name;
			ap->ismap=g->state->ismap;
			ap->imagebits=0;
			ap->field=field;
		}
	}
	plrtstr(&g->dst->text, space, indent, f->font, strdup(s), g->state->link!=0, ap);
	g->para=0;
	g->linebrk=0;
	g->dst->changed=1;
}
int pl_readc(Hglob *g){
	int n, c;
	char err[1024];
	do{
		if(g->hbufp==g->ehbuf){
			n=read(g->hfd, g->hbuf, NHBUF);
			if(n<=0){
				if(n<0){
					sprint(err, "%r reading %s", g->name);
					pl_htmloutput(g, 1, err, 0);
				}
				if(g->dst->url->cachefd!=-1)
					close(g->dst->url->cachefd);
				g->heof=1;
				return EOF;
			}
			if(g->dst->url->cachefd!=-1)
				write(g->dst->url->cachefd, g->hbuf, n);
			g->hbufp=g->hbuf;
			g->ehbuf=g->hbuf+n;
		}
		c=*g->hbufp++&255;
	}while(c=='\r');		/* dos machines? add spurious crs */
	if(c=='\n') g->lineno++;
	return c;
}
void pl_putback(Hglob *g, int c){
	if(g->npeekc==NPEEKC) htmlerror(g->name, g->lineno, "too much putback!");
	else if(c!=EOF) g->peekc[g->npeekc++]=c;
}
int pl_nextc(Hglob *g){
	int c;
	if(g->heof) return EOF;
	if(g->npeekc!=0) return g->peekc[--g->npeekc];
	c=pl_readc(g);
	if(c=='<'){
		c=pl_readc(g);
		if(c=='/'){
			c=pl_readc(g);
			pl_putback(g, c);
			pl_putback(g, '/');
			if('a'<=c && c<='z' || 'A'<=c && c<='Z') return STAG;
			return '<';
		}
		pl_putback(g, c);
		if(c=='!' || 'a'<=c && c<='z' || 'A'<=c && c<='Z') return STAG;
		return '<';
	}
	if(c=='>') return ETAG;
	return c;
}
int entchar(int c){
	return c=='#' || 'a'<=c && c<='z' || 'A'<=c && c<='Z' || '0'<=c && c<='9';
}
/*
 * remove entity references, in place.
 * Potential bug:
 *	This doesn't work if removing an entity reference can lengthen the string!
 *	Fortunately, this doesn't happen.
 */
void pl_rmentities(Hglob *g, char *s){
	char *t, *u, c;
	Pair *ep;
	char ebuf[4];
	Rune r;
	t=s;
	do{
		c=*s++;
		if(c=='&'
		&& ((*s=='#' && ('A'<=s[1] && s[1]<='Z' || '0'<=s[1] && s[1]<='9'))
		  || 'a'<=*s && *s<='z'
		  || 'A'<=*s && *s<='Z')){
			u=s;
			while(entchar(*s)) s++;
			if(*s!=';'){
				htmlerror(g->name, g->lineno, "entity syntax error");
				s=u;
				*t++='&';
			}
			else{
				*s++='\0';
				if(*u=='#' && '0'<=u[1] && u[1]<='9'){
					r=atoi(u+1);
					if(r==0xa0) r=' ';
					sprint(ebuf, "%C", r);
					u=ebuf;
				}
				else{
					for(ep=pl_entity;ep->name;ep++)
						if(strcmp(ep->name, u)==0)
							break;
					if(ep->name)
						u=ep->value;
					else{
						htmlerror(g->name, g->lineno,
							"unknown entity %s", u);
						s[-1]=';';
						s=u;
						*t++='&';
						u="";
					}
				}
				while(*u) *t++=*u++;
			}
		}	
		else *t++=c;
	}while(c);
}
/*
 * Skip over spaces
 */
char *pl_space(char *s){
	while(*s==' ' || *s=='\t' || *s=='\n' || *s=='\r') s++;
	return s;
}
/*
 * Skip over alphabetics
 */
char *pl_word(char *s){
	while('a'<=*s && *s<='z' || 'A'<=*s && *s<='Z' || '0'<=*s && *s<='9') s++;
	return s;
}
/*
 * Skip to matching quote
 */
char *pl_quote(char *s){
	char q;
	q=*s++;
	while(*s!=q && *s!='\0') s++;
	return s;
}
void pl_tagparse(Hglob *g, char *str){
	char *s, *t, *name, c;
	Pair *ap;
	Tag *tagp;
	ap=g->attr;
	if(str[0]=='!'){	/* test should be strncmp(str, "!--", 3)==0 */
		g->tag=Tag_comment;
		ap->name=0;
		return;
	}
	if(str[0]=='/') str++;
	name=str;
	s=pl_word(str);
	if(*s!=' ' && *s!='\n' && *s!='\t' && *s!='\0'){
		htmlerror(g->name, g->lineno, "bad tag name in %s", str);
		ap->name=0;
		return;
	}
	if(*s!='\0') *s++='\0';
	for(t=name;t!=s;t++) if('A'<=*t && *t<='Z') *t+='a'-'A';
	/*
	 * Binary search would be faster here
	 */
	for(tagp=tag;tagp->name;tagp++) if(strcmp(name, tagp->name)==0) break;
	g->tag=tagp-tag;
	if(g->tag==Tag_end) htmlerror(g->name, g->lineno, "no tag %s", name);
	for(;;){
		s=pl_space(s);
		if(*s=='\0'){
			ap->name=0;
			return;
		}
		ap->name=s;
		s=pl_word(s);
		t=pl_space(s);
		c=*t;
		*s='\0';
		for(s=ap->name;*s;s++) if('A'<=*s && *s<='Z') *s+='a'-'A';
		if(c=='='){
			s=pl_space(t+1);
			if(*s=='\'' || *s=='"'){
				ap->value=s+1;
				s=pl_quote(s);
				if(*s=='\0'){
					htmlerror(g->name, g->lineno,
						"No terminating quote in rhs of attribute %s",
						ap->name);
					ap->name=0;
					return;
				}
				*s++='\0';
			}
			else{
				/* read up to white space or > */
				ap->value=s;
				while(*s!=' ' && *s!='\t' && *s!='\n' && *s!='\0') s++;
				if(*s!='\0') *s++='\0';
			}
			pl_rmentities(g, ap->value);
		}
		else{
			if(c!='\0') s++;
			ap->value="";
		}
		if(ap==&g->attr[NATTR-1])
			htmlerror(g->name, g->lineno, "too many attributes!");
		else ap++;
	}
}
/*
 * Read a start or end tag -- the caller has read the initial <
 */
int pl_gettag(Hglob *g){
	char *tokp;
	int c;
	tokp=g->token;
	while((c=pl_nextc(g))!=ETAG && c!=EOF)
		if(tokp!=&g->token[NTOKEN-1]) *tokp++=c;
	*tokp='\0';
	if(c==EOF) htmlerror(g->name, g->lineno, "EOF in tag");
	pl_tagparse(g, g->token);
	if(g->token[0]!='/') return TAG;
	if(g->attr[0].name!=0)
		htmlerror(g->name, g->lineno, "end tag should not have attributes");
	return ENDTAG;
}
/*
 * The next token is a tag, an end tag or a sequence of
 * non-white characters.
 * If inside <pre>, newlines are converted to <br> and spaces are preserved.
 * Otherwise, spaces and newlines are noted and discarded.
 */
int pl_gettoken(Hglob *g){
	char *tokp;
	int c;
	if(g->state->pre) switch(c=pl_nextc(g)){
	case STAG: return pl_gettag(g);
	case EOF: return EOF;
	case '\n':
		pl_tagparse(g, "br");
		return TAG;
	default:
		tokp=g->token;
		do{
			if(c==ETAG) c='>';
			if(tokp!=&g->token[NTOKEN-1]) *tokp++=c;
			c=pl_nextc(g);
		}while(c!='\n' && c!=STAG && c!=EOF);
		*tokp='\0';
		pl_rmentities(g, g->token);
		pl_putback(g, c);
		g->nsp=0;
		g->spacc=0;
		return TEXT;
	}
	while((c=pl_nextc(g))==' ' || c=='\t' || c=='\n')
		if(g->spacc!=-1)
			g->spacc++;
	switch(c){
	case STAG: return pl_gettag(g);
	case EOF: return EOF;
	default:
		tokp=g->token;
		do{
			if(c==ETAG) c='>';
			if(tokp!=&g->token[NTOKEN-1]) *tokp++=c;
			c=pl_nextc(g);
		}while(c!=' ' && c!='\t' && c!='\n' && c!=STAG && c!=EOF);
		*tokp='\0';
		pl_rmentities(g, g->token);
		pl_putback(g, c);
		g->nsp=g->spacc;
		g->spacc=0;
		return TEXT;
	}
}
char *pl_getattr(Pair *attr, char *name){
	for(;attr->name;attr++)
		if(strcmp(attr->name, name)==0)
			return attr->value;
	return 0;
}
int pl_hasattr(Pair *attr, char *name){
	for(;attr->name;attr++)
		if(strcmp(attr->name, name)==0)
			return 1;
	return 0;
}
#define	NLINE	256
void plaintext(Hglob *g){
	char line[NLINE];
	char *lp, *elp;
	int c;
	g->state->font=CWIDTH;
	g->state->size=NORMAL;
	g->state->pre=1;
	elp=&line[NLINE+1];
	lp=line;
	for(;;){
		c=pl_readc(g);
		if(c==EOF) break;
		if(c=='\n' || lp==elp){
			*lp='\0';
			pl_htmloutput(g, 0, line, 0);
			lp=line;
		}
		if(c!='\n')
			*lp++=c;
	}
	if(lp!=line){
		*lp='\0';
		pl_htmloutput(g, 0, line, 0);
	}
}
void plrdplain(char *name, int fd, Www *dst){
	Hglob g;
	g.state=g.stack;
	g.state->tag=Tag_html;
	g.state->font=CWIDTH;
	g.state->size=NORMAL;
	g.state->pre=0;
	g.state->image=0;
	g.state->link=0;
	g.state->name=0;
	g.state->margin=0;
	g.state->indent=20;
	g.state->ismap=0;
	g.dst=dst;
	g.hfd=fd;
	g.name=name;
	g.ehbuf=g.hbufp=g.hbuf;
	g.npeekc=0;
	g.heof=0;
	g.lineno=1;
	g.linebrk=1;
	g.para=0;
	g.text=dst->title;
	g.tp=g.text;
	g.etext=g.text+NTITLE-1;
	g.spacc=0;
	g.form=0;
	strncpy(g.text, name, NTITLE);
	plaintext(&g);
	dst->finished=1;
}
void plrdhtml(char *name, int fd, Www *dst){
	Stack *sp;
	char buf[20];
	char *str;
	Hglob g;
	int tagerr;
	g.state=g.stack;
	g.state->tag=Tag_html;
	g.state->font=ROMAN;
	g.state->size=NORMAL;
	g.state->pre=0;
	g.state->image=0;
	g.state->link=0;
	g.state->name=0;
	g.state->margin=0;
	g.state->indent=25;
	g.state->ismap=0;
	g.dst=dst;
	g.hfd=fd;
	g.name=name;
	g.ehbuf=g.hbufp=g.hbuf;
	g.npeekc=0;
	g.heof=0;
	g.lineno=1;
	g.linebrk=1;
	g.para=0;
	g.text=dst->title;
	g.tp=g.text;
	g.etext=g.text+NTITLE-1;
	g.spacc=0;
	g.form=0;
	for(;;) switch(pl_gettoken(&g)){
	case TAG:
		switch(tag[g.tag].action){
		case OPTEND:
			for(sp=g.state;sp!=g.stack && sp->tag!=g.tag;--sp);
			if(sp->tag!=g.tag)
				pl_pushstate(&g, g.tag);
			else
				for(;g.state!=sp;--g.state)
					if(tag[g.state->tag].action!=OPTEND)
						htmlerror(g.name, g.lineno,
							"end tag </%s> missing",
							tag[g.state->tag].name);
			break;
		case END:
			pl_pushstate(&g, g.tag);
			break;
		}
		switch(g.tag){
		default:
			htmlerror(g.name, g.lineno,
				"unimplemented tag <%s>", tag[g.tag].name);
			break;
		case Tag_end:	/* unrecognized start tag */
			break;
		case Tag_img:
			str=pl_getattr(g.attr, "src");
			if(str) g.state->image=strdup(str);
			g.state->ismap=pl_hasattr(g.attr, "ismap");
			str=pl_getattr(g.attr, "alt");
			if(str==0){
				if(g.state->image)
					str=strdup(g.state->image);
				else
					str="[[image]]";
			}
			pl_htmloutput(&g, 0, str, 0);
			g.state->image=0;
			g.state->ismap=0;
			break;
		case Tag_plaintext:
			g.spacc=0;
			plaintext(&g);
			break;
		case Tag_comment:
		case Tag_html:
		case Tag_link:
		case Tag_nextid:
			break;
		case Tag_a:
			str=pl_getattr(g.attr, "href");
			if(str)
				g.state->link=strdup(str);
			str=pl_getattr(g.attr, "name");
			if(str){
				g.state->name=strdup(str);
				pl_htmloutput(&g, 0, "", 0);
			}
			break;
		case Tag_address:
			g.spacc=0;
			g.linebrk=1;
			g.state->font=ROMAN;
			g.state->size=NORMAL;
			g.state->margin=300;
			g.state->indent=50;
			break;
		case Tag_b:
		case Tag_strong:
			g.state->font=BOLD;
			break;
		case Tag_blockquot:
			g.spacc=0;
			g.linebrk=1;
			g.state->margin+=50;
			g.state->indent=20;
			break;
		case Tag_head:
		case Tag_body:
			g.state->font=ROMAN;
			g.state->size=NORMAL;
			g.state->margin=0;
			g.state->indent=20;
			g.spacc=0;
			break;
		case Tag_br:
			g.spacc=0;
			g.linebrk=1;
			break;
		case Tag_cite:
			g.state->font=ITALIC;
			g.state->size=NORMAL;
			break;
		case Tag_code:
			g.state->font=CWIDTH;
			g.state->size=NORMAL;
			break;
		case Tag_dd:
			g.linebrk=1;
			g.state->font=ROMAN;
			g.spacc=0;
			break;
		case Tag_dfn:
			htmlerror(g.name, g.lineno, "<dfn> deprecated");
			g.state->font=BOLD;
			g.state->size=NORMAL;
			break;
		case Tag_dl:
			g.state->font=BOLD;
			g.state->size=NORMAL;
			g.state->margin=40;
			g.state->indent=-40;
			g.spacc=0;
			break;
		case Tag_dt:
			g.para=1;
			g.state->font=BOLD;
			g.spacc=0;
			break;
		case Tag_u:
			htmlerror(g.name, g.lineno, "<u> deprecated");
		case Tag_em:
		case Tag_i:
		case Tag_var:
			g.state->font=ITALIC;
			break;
		case Tag_h1:
			g.linebrk=1;
			g.state->font=BOLD;
			g.state->size=ENORMOUS;
			g.state->margin+=100;
			g.spacc=0;
			break;
		case Tag_h2:
			pl_linespace(&g);
			g.state->font=BOLD;
			g.state->size=ENORMOUS;
			g.spacc=0;
			break;
		case Tag_h3:
			g.linebrk=1;
			pl_linespace(&g);
			g.state->font=ITALIC;
			g.state->size=ENORMOUS;
			g.state->margin+=20;
			g.spacc=0;
			break;
		case Tag_h4:
			pl_linespace(&g);
			g.state->font=BOLD;
			g.state->size=LARGE;
			g.state->margin+=10;
			g.spacc=0;
			break;
		case Tag_h5:
			pl_linespace(&g);
			g.state->font=ITALIC;
			g.state->size=LARGE;
			g.state->margin+=10;
			g.spacc=0;
			break;
		case Tag_h6:
			pl_linespace(&g);
			g.state->font=BOLD;
			g.state->size=LARGE;
			g.spacc=0;
			break;
		case Tag_hr:
			g.spacc=0;
			plrtbitmap(&g.dst->text, 1000000, g.state->margin, hrule, 0, 0);
			break;
		case Tag_key:
			htmlerror(g.name, g.lineno, "<key> deprecated");
		case Tag_kbd:
			g.state->font=CWIDTH;
			break;
		case Tag_dir:
		case Tag_menu:
		case Tag_ol:
		case Tag_ul:
			g.state->number=0;
			g.linebrk=1;
			g.state->margin+=25;
			g.state->indent=-25;
			g.spacc=0;
			break;
		case Tag_li:
			g.spacc=0;
			switch(g.state->tag){
			default:
				htmlerror(g.name, g.lineno, "can't have <li> in <%s>",
					tag[g.state->tag].name);
			case Tag_dir:	/* supposed to be multi-columns, can't do! */
			case Tag_menu:
				g.linebrk=1;
				break;
			case Tag_ol:
				g.para=1;
				sprint(buf, "%2d  ", ++g.state->number);
				pl_htmloutput(&g, 0, buf, 0);
				break;
			case Tag_ul:
				g.para=0;
				g.linebrk=0;
				g.spacc=-1;
				plrtbitmap(&g.dst->text, 100000,
					g.state->margin+g.state->indent, bullet, 0, 0);
				break;
			}
			break;
		case Tag_p:
			g.para=1;
			g.spacc=0;
			break;
		case Tag_listing:
		case Tag_xmp:
			htmlerror(g.name, g.lineno, "<%s> deprecated", tag[g.tag].name);
		case Tag_pre:
		case Tag_samp:
			g.state->pre=1;
			g.state->font=CWIDTH;
			g.state->size=NORMAL;
			break;
		case Tag_tt:
			g.state->font=CWIDTH;
			g.state->size=NORMAL;
			break;
		case Tag_title:
			break;
		case Tag_form:
		case Tag_input:
		case Tag_select:
		case Tag_option:
		case Tag_textarea:
		case Tag_isindex:
			rdform(&g);
			break;
		}
		break;
	case ENDTAG:
		/*
		 * If the end tag doesn't match the top, we try to uncover a match
		 * on the stack.
		 */
		if(g.state->tag!=g.tag){
			tagerr=0;
			for(sp=g.state;sp!=g.stack;--sp){
				if(sp->tag==g.tag)
					break;
				if(tag[g.state->tag].action!=OPTEND) tagerr++;
			}
			if(sp==g.stack){
				if(tagerr)
					htmlerror(g.name, g.lineno,
						"end tag mismatch <%s>...</%s>, ignored",
						tag[g.state->tag].name, tag[g.tag].name);
			}
			else{
				if(tagerr)
					htmlerror(g.name, g.lineno,
						"end tag mismatch <%s>...</%s>, "
						"intervening tags popped",
						tag[g.state->tag].name, tag[g.tag].name);
				g.state=sp-1;
			}
		}
		else if(g.state==g.stack)
			htmlerror(g.name, g.lineno, "end tag </%s> at stack bottom",
				tag[g.tag].name);
		else
			--g.state;
		switch(g.tag){
		case Tag_select:
		case Tag_form:
		case Tag_textarea:
			endform(&g);
			break;
		case Tag_h1:
		case Tag_h2:
		case Tag_h3:
		case Tag_h4:
			pl_linespace(&g);
			break;
		case Tag_address:
		case Tag_blockquot:
		case Tag_body:
		case Tag_dir:
		case Tag_dl:
		case Tag_dt:
		case Tag_h5:
		case Tag_h6:
		case Tag_listing:
		case Tag_menu:
		case Tag_ol:
		case Tag_pre:
		case Tag_samp:
		case Tag_title:
		case Tag_ul:
		case Tag_xmp:
			g.linebrk=1;
			break;
		}
		break;
	case TEXT:
		pl_htmloutput(&g, g.nsp, g.token, 0);
		break;
	case EOF:
		for(;g.state!=g.stack;--g.state)
			if(tag[g.state->tag].action!=OPTEND)
				htmlerror(g.name, g.lineno,
					"missing </%s> at EOF", tag[g.state->tag].name);
		*g.tp='\0';
		dst->changed=1;
		getpix(dst->text, dst);
		dst->finished=1;
		return;
	}
}

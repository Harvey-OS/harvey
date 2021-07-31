#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include <ctype.h>
#include "mothra.h"
#define	IP	1	/* url can contain //ipaddress[:port] */
#define	REL	2	/* fill in ip address & root of name from current, if necessary */
Scheme scheme[]={
	"http:",	HTTP,	IP|REL,	80,
	"ftp:",		FTP,	IP,	21,
	"file:",	FILE,	REL,	0,
	"telnet:",	TELNET,	IP,	0,
	"mailto:",	MAILTO,	0,	0,
	"gopher:",	GOPHER,	IP,	70,
#ifdef securityHole
	"exec:",	EXEC,	0,	0,
#endif
	0,		HTTP,	IP|REL,	80,
};
int endaddr(int c){
	return c=='/' || c==':' || c=='?' || c=='#' || c=='\0';
}
/*
 * Remove ., mu/.. and empty components from path names.
 * Empty last components of urls are significant, and
 * therefore preserved.
 */
void urlcanon(char *name){
	char *s, *t;
	char **comp, **p, **q;
	int rooted;
	rooted=name[0]=='/';
	/*
	 * Break the name into a list of components
	 */
	comp=emalloc(strlen(name)*sizeof(char *));
	p=comp;
	*p++=name;
	for(s=name;;s++){
		if(*s=='/'){
			*p++=s+1;
			*s='\0';
		}
		else if(*s=='\0')
			break;
	}
	*p=0;
	/*
	 * go through the component list, deleting components that are empty (except
	 * the last component) or ., and any .. and its non-.. predecessor.
	 */
	p=q=comp;
	while(*p){
		if(strcmp(*p, "")==0 && p[1]!=0
		|| strcmp(*p, ".")==0)
			p++;
		else if(strcmp(*p, "..")==0 && q!=comp && strcmp(q[-1], "..")!=0){
			--q;
			p++;
		}
		else
			*q++=*p++;
	}
	*q=0;
	/*
	 * rebuild the path name
	 */
	s=name;
	if(rooted) *s++='/';
	for(p=comp;*p;p++){
		t=*p;
		while(*t) *s++=*t++;
		if(p[1]!=0) *s++='/';
	}
	*s='\0';
	free(comp);
}
/*
 * True url parsing is a nightmare.
 * This assumes that there are two basic syntaxes
 * for url's -- with and without an ip address.
 * If the type identifier is missing, it assumes that the type is the same as cur.
 * If the relative address is empty, it is assumed to be the same as cur.
 */
void crackurl(Url *url, char *urlname, Url *cur){
	char *relp, *tagp;
	int len;
	Scheme *up;
	char buf[30];
	/*
	 * The following lines `fix' the most egregious urlname syntax errors
	 */
	while(*urlname==' ' || *urlname=='\t' || *urlname=='\n') urlname++;
	relp=strchr(urlname, '\n');
	if(relp) *relp='\0';
	*url=*cur;
	if(strchr(urlname, ':')==0){
		up=cur->scheme;
		if(up==0){
			up=&scheme[0];
			cur->scheme=up;
		}
	}
	else{
		for(up=scheme;up->name;up++){
			len=strlen(up->name);
			if(strncmp(urlname, up->name, len)==0){
				urlname+=len;
				break;
			}
		}
		if(up->name==0) up=&scheme[0];	/* default to http: */
	}
	url->access=up->type;
	url->scheme=up;
	if(up!=cur->scheme)
		url->reltext[0]='\0';
	if(up->flags&IP && strncmp(urlname, "//", 2)==0){
		urlname+=2;
		for(relp=urlname;!endaddr(*relp);relp++);
		len=relp-urlname;
		strncpy(url->ipaddr, urlname, len);
		url->ipaddr[len]='\0';
		urlname=relp;
		if(*urlname==':'){
			urlname++;
			url->port=atoi(urlname);
			while(!endaddr(*urlname)) urlname++;
		}
		else
			url->port=up->port;
		if(*urlname=='\0') urlname="/";
	}
	tagp=strchr(urlname, '#');
	if(tagp){
		*tagp='\0';
		strcpy(url->tag, tagp+1);
	}
	else
		url->tag[0]='\0';	
	if(!(up->flags&REL) || *urlname=='/')
		strcpy(url->reltext, urlname);
	else if(urlname[0]){
		relp=strrchr(url->reltext, '/');
		if(relp==0)
			strcpy(url->reltext, urlname);
		else
			strcpy(relp+1, urlname);
	}
	urlcanon(url->reltext);
	if(tagp) *tagp='#';
	/*
	 * The following mess of strcpys and strcats
	 * can't be changed to a few sprints because
	 * urls are not necessarily composed of legal utf
	 */
	strcpy(url->fullname, up->name);
	if(up->flags&IP){
		strcat(url->fullname, "//");
		strcat(url->fullname, url->ipaddr);
		if(url->port!=up->port){
			sprint(buf, ":%d", url->port);
			strcat(url->fullname, buf);
		}
	}
	strcat(url->fullname, url->reltext);
}

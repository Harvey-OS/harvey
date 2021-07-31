#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#include <panel.h>
#include "mothra.h"
void httpheader(Url *, char *);
int httpresponse(char *);
/*
 * Given a url, return a file descriptor on which caller can
 * read an http document.  As a side effect, we parse the
 * http header and fill in some fields in the url.
 * The caller is responsible for processing redirection loops.
 * Method can be either GET or POST.  If method==post, body
 * is the text to be posted.
 */
int http(Url *url, int method, char *body){
	char *addr, *com;
	int fd, n, nnl, len;
	int pfd[2];
	char buf[1024], *bp, *ebp;
	char line[1024+1], *lp, *elp;
	int first;
	int response;
	addr=emalloc(strlen(url->ipaddr)+100);
	sprint(addr, "tcp!%s!%d", url->ipaddr, url->port);
	fd=dial(addr, 0, 0, 0);
	free(addr);
	if(fd==-1) return -1;
	com=emalloc(strlen(url->reltext)+120);
	switch(method){
	case GET:
		n=sprint(com, "GET %s HTTP/1.0\r\n\r\n", url->reltext);
		if(write(fd, com, n)!=n){
			free(com);
			return -1;
		}
		break;
	case POST:
		len=strlen(body);
		n=sprint(com,
			"POST %s HTTP/1.0\r\n"
			"Content-type: application/x-www-form-urlencoded\r\n"
			"Content-length: %d\r\n\r\n",
			url->reltext, len);
		if(write(fd, com, n)!=n
		|| write(fd, body, len)!=len){
			free(com);
			return -1;
		}
		break;
	}
	free(com);
	if(pipe(pfd)==-1){
		close(fd);
		return -1;
	}
	n=read(fd, buf, 1024);
	if(n<=0){
	EarlyEof:
		if(n<0) fprint(2, "%s: %n", url->fullname);
		else fprint(2, "%s: EOF in header\n", url->fullname);
		close(fd);
		return -1;
	}
	bp=buf;
	ebp=buf+n;
	url->type=0;
	if(strncmp(buf, "HTTP/", 5)==0){	/* hack test for presence of header */
		SET(response);
		first=1;
		url->redirname[0]='\0';
		nnl=0;
		lp=line;
		elp=line+1024;
		while(nnl!=2){
			if(bp==ebp){
				n=read(fd, buf, 1024);
				if(n<=0) goto EarlyEof;
				ebp=buf+n;
				bp=buf;
			}
			if(*bp!='\r'){
				if(nnl==1 && (first || (*bp!=' ' && *bp!='\t'))){
					*lp='\0';
					if(first){
						response=httpresponse(line);
						first=0;
					}
					else
						httpheader(url, line);
					lp=line;
				}
				if(*bp=='\n') nnl++;
				else{
					nnl=0;
					if(lp!=elp) *lp++=*bp;
				}
			}
			bp++;
		}
		if(!first && (response==301 || response==302) && url->redirname[0]){
			close(pfd[0]);
			close(pfd[1]);
			close(fd);
			url->type=FORWARD;
			return -1;
		}
	}
	if(url->type==0) url->type=HTML;
	switch(rfork(RFFDG|RFPROC|RFNOWAIT)){
	case -1:
		werrstr("Can't fork");
		close(pfd[0]);
		close(pfd[1]);
		return -1;
	case 0:
		close(pfd[0]);
		if(bp!=ebp)
			write(pfd[1], bp, ebp-bp);
		while((n=read(fd, buf, 1024))>0)
			write(pfd[1], buf, n);
		if(n<0) fprint(2, "%s: %r\n", url->fullname);
		_exits(0);
	default:
		close(pfd[1]);
		close(fd);
		return pfd[0];
	}
}
/*
 * Process a header line for this url
 */
void httpheader(Url *url, char *line){
	char *name, *arg, *s;
	name=line;
	for(s=line;*s!=':';s++) if(*s=='\0') return;
	*s++='\0';
	while(*s==' ' || *s=='\t') s++;
	arg=s;
	while(*s!=' ' && *s!='\t' && *s!=';' && *s!='\0') s++;
	*s='\0';
	if(cistrcmp(name, "Content-Type")==0)
		url->type|=content2type(arg);
	else if(cistrcmp(name, "Content-Encoding")==0)
		url->type|=encoding2type(arg);
	else if(cistrcmp(name, "URI")==0){
		if(*arg!='<') return;
		++arg;
		for(s=arg;*s!='>';s++) if(*s=='\0') return;
		*s='\0';
		strncpy(url->redirname, arg, sizeof(url->redirname));
	}
	else if(cistrcmp(name, "Location")==0)
		strncpy(url->redirname, arg, sizeof(url->redirname));
}
int httpresponse(char *line){
	while(*line!=' ' && *line!='\t' && *line!='\0') line++;
	return atoi(line);
}

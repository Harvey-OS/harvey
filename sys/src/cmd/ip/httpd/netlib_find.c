/* invoked from /netlib/pub/search.html */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include "httpd.h"
#include "httpsrv.h"

void bib_fmt(char*,char*);
void index_fmt(char*,char*);
void no_fmt(char*,char*);
int send(HConnect*);

Hio *hout;

/********** table of databases ************/

typedef struct DB	DB;
struct DB 
{
	int	SELECT;	/* value from search.html */
	char	*log;	/* abbreviation for logfile */
	int	maxhit;	/* maximum number of hits to return */
	char	*file;	/* searchfs database */
	void	(*fmt)(char*,char*); /* convert one record to HTML */
	char	*postlude;	/* trailer text */
};

DB db[] =
{
 {0, "netlib",	250, "/srv/netlib_DEFAULT", index_fmt,
	"<HR><A HREF=\"/netlib/master\">browse netlib</A></BODY>\r\n"},
 {1, "BibNet",	250, "/srv/netlib_bibnet", bib_fmt,
	"<HR><A HREF=\"/netlib/bibnet\">browse BibNet</A></BODY>\r\n"},
 {2, "compgeom",	250, "/srv/netlib_compgeom", no_fmt, "</BODY>\r\n"},
 {3, "approx",	250, "/srv/netlib_approximation", no_fmt,
	"<HR><A HREF=\"/netlib/a/catalog.html.gz\">hierarchical catalog</A></BODY>\r\n"},
 {4, "siam",	 50, "/srv/netlib_siam-Secret", no_fmt, "</BODY>\r\n"},
 {-1,"",0,"",no_fmt,""}
};



/********** reformat database record as HTML ************/

void /* tr '\015' '\012' ("uncombline") */
no_fmt(char*s,char*e)
{
	/* s = start, e = (one past) end of database record */
	char *p;
	for(p = s; p<e; p++)
		if(*p=='\r'){
			hwrite(hout, s,p-s);
			hprint(hout, "\n");
			s = p+1;
		}
}

int /* should the filename have .gz appended? */
suffix(char*filename)
{
	int n;
	char *z;

	if(!filename || *filename==0)
		return(0);
	n = strlen(filename);
	if(strncmp(".html",filename+n-5,5)==0)
		return(0);
	z = malloc(n+50);
	if(z == nil)
		return(0);
	strcpy(z,"/netlib/pub/");
	strcat(z,filename);
	strcat(z,".gz");
	if(access(z,4)==0){
		free(z);
		return(1);
	}
	free(z);
	return(0);
}

void /* add HREF to "file:" lines */
index_fmt(char*s,char*e)
{
	char *p, *filename;
	if(strncmp(s,"file",4)==0 && (s[4]==' '||s[4]=='\t')){
		for(filename = s+4; strchr(" \t",*filename); filename++){}
		for(s = filename; *s && strchr("\r\n",*s)==nil; s++){}
		*s++ = '\0';
		if(*s=='\n') s++;
		hprint(hout, "file:   <A HREF=\"/netlib/%s",filename);
		if(suffix(filename))
			hprint(hout, ".gz");
		hprint(hout, "\">%s</A>\r\n",filename);
		for(p = s; p<e; p++)
			if(*p=='\r'){
				hwrite(hout, s,p-s);
				hprint(hout, "\n");
				s = p+1;
			}
	}else if(strncmp(s,"lib",3)==0 && (s[3]==' '||s[3]=='\t')){
		for(filename = s+3; strchr(" \t",*filename); filename++){}
		for(s = filename; *s && strchr("\r\n",*s)==nil; s++){}
		*s++ = '\0';
		if(*s=='\n') s++;
		hprint(hout, "lib:    <A HREF=\"/netlib/%s",filename);
		hprint(hout, "\">%s</A>\r\n",filename);
		for(p = s; p<e; p++)
			if(*p=='\r'){
				hwrite(hout, s,p-s);
				hprint(hout, "\n");
				s = p+1;
			}
	}else{
		no_fmt(s,e);
	}
}

void /* add HREF to "URL" lines */
bib_fmt(char*s,char*e)
{
	char *p, *filename;
	for(p = s; p<e; p++)
		if(*p=='\r'){
			hwrite(hout, s,p-s);
			hprint(hout, "\n");
			s = p+1;
			if(strncmp(s," URL =",6)==0 &&
					(filename = strchr(s+6,'"'))!=nil){
				filename++;
				for(s = filename; *s && strchr("\"\r\n",*s)==nil; s++){}
				*s++ = '\0';
				p = s;
				hprint(hout, " URL =<A HREF=\"%s\">%s</A>",
					filename,filename);
			}
		}
}


/********** main() calls httpheadget() calls send() ************/

void
main(int argc, char **argv)
{
	HConnect *c;

	c = init(argc, argv);
	hout = &c->hout;
	if(hparseheaders(c, HSTIMEOUT) >= 0)
		send(c);
	exits(nil);
}

Biobuf Blist;

Biobuf*
init800fs(char*name,char*pat)
{
	int fd800fs, n;
	char*search;

	fd800fs = open(name, ORDWR);
	if(fd800fs < 0)
		exits("can't connect to 800fs server");
	if(mount(fd800fs, -1, "/mnt", MREPL, "") < 0)
		exits("can't mount /mnt");
	fd800fs = open("/mnt/search", ORDWR);
	n = strlen("search=")+strlen(pat)+1;
	search = ezalloc(n);
	strcpy(search,"search=");
	strcat(search,pat);
	write(fd800fs,search,n);
	free(search);
	Binit(&Blist, fd800fs,OREAD);
	return(&Blist);
}


static char *
hq(char *text)
{
	int textlen = strlen(text), escapedlen = textlen;
	char *escaped, *s, *w;

	for(s = text; *s; s++)
		if(*s=='<' || *s=='>' || *s=='&')
			escapedlen += 4;
	escaped = ezalloc(escapedlen+1);
	for(s = text, w = escaped; *s; s++){
		if(*s == '<'){
			strcpy(w, "&lt;");
			w += 4;
		}else if(*s == '>'){
			strcpy(w, "&gt;");
			w += 4;
		}else if(*s == '&'){
			strcpy(w, "&amp;");
			w += 5;
		}else{
			*w++ = *s;
		}
	}
	return escaped;
}

int
send(HConnect *c)
{
	Biobuf*blist;
	int m, n, dbi, nmatch;
	char *pat, *s, *e;
	HSPairs *q;

	if(strcmp(c->req.meth, "GET") != 0 && strcmp(c->req.meth, "HEAD") != 0)
		return hunallowed(c, "GET, HEAD");
	if(c->head.expectother || c->head.expectcont)
		return hfail(c, HExpectFail, nil);
	if(c->req.search == nil || !*c->req.search)
		return hfail(c, HNoData, "netlib_find");
	s = c->req.search;
	while((s = strchr(s, '+')) != nil)
		*s++ = ' ';
	dbi = -1;
	pat = nil;
	for(q = hparsequery(c, hstrdup(c, c->req.search)); q; q = q->next){
		if(strcmp(q->s, "db") == 0){
			m = atoi(q->t);
			for(dbi = 0; m!=db[dbi].SELECT; dbi++)
				if(db[dbi].SELECT<0)
					exits("unrecognized db");
		}else if(strcmp(q->s, "pat") == 0){
			pat = q->t;
		}
	}
	if(dbi < 0)
		exits("missing db field in query");
	if(pat == nil)
		exits("missing pat field in query");
	logit(c, "netlib_find %s %s", db[dbi].log,pat);

	blist = init800fs(db[dbi].file,pat);

	if(c->req.vermaj){
		hokheaders(c);
		hprint(hout, "Content-type: text/html\r\n");
		hprint(hout, "\r\n");
	}
	if(strcmp(c->req.meth, "HEAD") == 0){
		writelog(c, "Reply: 200 netlib_find 0\n");
		hflush(hout);
		exits(nil);
	}

	hprint(hout, "<HEAD><TITLE>%s/%s</TITLE></HEAD>\r\n<BODY>\r\n",
		db[dbi].log,hq(pat));
	nmatch = 0;

	while(s = Brdline(blist, '\n')){ /* get next database record */
		n = Blinelen(blist);
		e = s+n;
		hprint(hout, "<PRE>");
		(*db[dbi].fmt)(s,e);
		hprint(hout, "</PRE>\r\n");
		if(nmatch++>=db[dbi].maxhit){
			hprint(hout, "<H4>reached limit at %d hits</H4>\n\r",nmatch);
			break;
		}
	}
	if(nmatch==0)
		hprint(hout, "<H4>Nothing Found.</H4>\r\n");
	hprint(hout, db[dbi].postlude);
	hflush(hout);
	writelog(c, "Reply: 200 netlib_find %ld %ld\n", hout->seek, hout->seek);
	return 1;
}

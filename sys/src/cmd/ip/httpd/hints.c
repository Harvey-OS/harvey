#include <u.h>
#include <libc.h>
#include <bio.h>
#include "httpd.h"
#include "httpsrv.h"

enum{ URLmax = 65536, HINTmax = 20 };
#define RECIPLOG2 1.44269504089

char **urlname;				/* array of url strings    1,...,nurl */
static int nurl;
static uint urltab[URLmax];		/* hashstr(url)  1,...,nurl */
static int urlnext[URLmax];		/* index urltab of next url in chain */
static int urlhash[URLmax];		/* initially 0, meaning empty buckets */

typedef struct Hint {
	ushort url;
	uchar prob;
} Hint;
Hint *hints[URLmax];
uchar nhint[URLmax];

vlong
Bfilelen(void *vb)
{
	Biobuf *b;
	vlong n;

	b = vb;
	n = Bseek(b, 0L, 2);
	Bseek(b, 0L, 0);
	return n;
}

static uint 
hashstr(char* key)
{
	/* asu works better than pjw for urls */
	uchar *k = (unsigned char*)key;
	uint h = 0;
	while(*k!=0)
		h = 65599*h + *k++;
        return h;
}

static int
urllookup(uint url)
{
	/* returns +index into urltab, else -hash */
	int j, hash;

	hash = 1 + url%(URLmax-1);
	j = urlhash[hash];
	for(;;){
		if(j==0)
			return -hash;
		if(url==urltab[j])
			return j;
		j = urlnext[j];
	}
}

int
Bage(Biobuf *b)
{
	Dir *dir;
	long mtime;

	dir = dirfstat(Bfildes(b));
	if(dir != nil)
		mtime = dir->mtime;
	else
		mtime = 0;
	free(dir);
	return time(nil) - mtime;
}

void
urlinit(void)
{
	static Biobuf *b = nil;
	static vlong filelen = 0;
	vlong newlen;
	char *s, *arena;
	int i, j, n;
	uint url;
	char *file;

	if(filelen < 0)
		return;
	file = "/sys/log/httpd/url";
	if(b == nil){
		b = Bopen(file, OREAD); /* first time */
		if(b == nil){
			syslog(0, HTTPLOG, "no %s, abandon prefetch hints", file);
			filelen = -1;
			return;
		}
	}
	newlen = Bfilelen(b); /* side effect: rewinds b */
	if(newlen == filelen || Bage(b)<300)
		return;
	filelen = newlen;
	if(filelen < 0)
		return;
	if(nurl){ /* free existing tables */
		free(urlname[0]); /* arena */
		memset(urlhash,0,sizeof urlhash);
		memset(urlnext,0,sizeof urlnext);
		nurl = 0;
	}
	if(urlname==nil)
		urlname = (char**)ezalloc(URLmax*sizeof(*urlname));
	arena = (char*)ezalloc(filelen);  /* enough for all the strcpy below */
	i = 1;
	while((s=Brdline(b,'\n'))!=0){
		/* read lines of the form:  999 /url/path */
		n = Blinelen(b) - 1;
		if(n>2 && s[n]=='\n'){
			s[n] = '\0';
		}else{
			sysfatal("missing fields or newline in url-db");
		}
		j = strtoul(s,&s,10);
		while(*s==' ')
			s++;
		if(i++!=j)
			sysfatal("url-db synchronization error");
		url = hashstr(s);
		j = urllookup(url);
		if(j>=0)
			sysfatal("duplicate url");
		j = -j;
		nurl++;
		if(nurl>=URLmax){
			syslog(0, HTTPLOG, "urlinit overflow at %s",s);
			free(urlname[0]); /* arena */
			memset(urlhash,0,sizeof urlhash);
			memset(urlnext,0,sizeof urlnext);
			nurl = 0;
			return;
		}
		urltab[nurl] = url;
		urlnext[nurl] = urlhash[j];
		urlhash[j] = nurl;
		strcpy(arena,s);
		urlname[nurl] = arena;
		arena += strlen(s)+1;
	}
	syslog(0, HTTPLOG, "prefetch-hints url=%d (%.1fMB)", nurl, 1.e-6*(URLmax*sizeof(*urlname)+filelen));
	/* b is held open, because namespace will be chopped */
}

void
statsinit(void)
{
	static Biobuf *b = nil;
	static vlong filelen = 0;
	vlong newlen;
	int iq, n, i, nstats = 0;
	uchar *s, buf[3+HINTmax*3];  /* iq, n, (url,prob)... */
	Hint *arena, *h;
	char *file;
	static void *oldarena = nil;

	file = "/sys/log/httpd/pathstat";
	if(b == nil){
		if(filelen == -1)
			return; /* if failed first time */
		b = Bopen(file, OREAD); /* first time */
		if(b == nil){
			syslog(0, HTTPLOG, "no %s, abandon prefetch hints", file);
			filelen = -1;
			return;
		}
	}
	newlen = Bfilelen(b); /* side effect: rewinds b */
	if(newlen == filelen || Bage(b)<300)
		return;
	filelen = newlen;
	if(oldarena){
		free(oldarena);
		memset(nhint,0,sizeof nhint);
	}
	arena = (Hint*)ezalloc((filelen/3)*sizeof(Hint));
	oldarena = arena;
	for(;;){
		i = Bread(b,buf,3);
		if(i<3)
			break;
		nstats++;
		iq = buf[0];
		iq = (iq<<8) | buf[1];
		n = buf[2];
		h = arena;
		arena += n;
		hints[iq] = h;
		nhint[iq] = n;
		if(Bread(b,buf,3*n)!=3*n)
			sysfatal("stats read error");
		for(i=0; i<n; i++){
			s = &buf[3*i];
			h[i].url = (s[0]<<8) | s[1];
			h[i].prob = s[2];
		}
	}
	syslog(0, HTTPLOG, "prefetch-hints stats=%d (%.1fMB)", nstats, 1.e-6*((filelen/3)*sizeof(Hint)));
}

void
urlcanon(char *url)
{
	/* all the changes here can be implemented by rewriting in-place */
	char *p, *q;

	/* remove extraneous '/' in the middle and at the end */
	p = url+1;  /* first char needs no change */
	q = p;
	while(q[0]){
		if(q[0]=='/' && q[-1]=='/'){
			q++;
			continue;
		}
		*p++ = *q++;
	}
	if(q[-1]=='/'){  /* trailing '/' */
		p[-1] = '\0';
	}else{
		p[0] = '\0';
	}

	/* specific to the cm.bell-labs.com web site */
	if(strncmp(url,"/cm/",4)==0){
		if(strchr("cims",url[4]) && strncmp(url+5,"s/who/",6)==0)
			/* strip off /cm/cs */
			memmove(url,url+6,strlen(url+6)+1);
		else if(strncmp(url+4,"ms/what/wavelet",15)==0)
			/* /cm/ms/what */
			memmove(url,url+11,strlen(url+11)+1);
	}
}

void
hintprint(HConnect *hc, Hio *hout, char *uri, int thresh, int havej)
{
	int i, j, pr, prefix, fd, siz, havei, newhint = 0, n;
	char *query, *sf, etag[32], *wurl;
	Dir *dir;
	Hint *h, *haveh;

	query = hstrdup(hc, uri);
	urlcanon(query);
	j = urllookup(hashstr(query));
	if(j < 0)
		return;
	query = strrchr(uri,'/');
	if(!query)
		return;  /* can't happen */
	prefix = query-uri+1;  /* = strlen(dirname)+1 */
	h = hints[j];
	for(i=0; i<nhint[j]; i++){
		if(havej > 0 && havej < URLmax){ /* exclude hints client has */
			haveh = hints[havej];
			for(havei=0; havei<nhint[havej]; havei++)
				if( haveh[havei].url == h[i].url)
					goto continuei;
		}
		sf = urlname[h[i].url];
		pr = h[i].prob;
		if(pr<thresh)
			break;
		n = strlen(webroot) + strlen(sf) + 1;
		wurl = halloc(hc, n);
		strcpy(wurl, webroot);
		strcat(wurl, sf);
		fd = open(wurl, OREAD);
		if(fd<0)
			continue;
		dir = dirfstat(fd);
		if(dir == nil){
			close(fd);
			continue;
		}
		close(fd);
		snprint(etag, sizeof(etag), "\"%lluxv%lux\"", dir->qid.path, dir->qid.vers);
		siz = (int)( log((double)dir->length) * RECIPLOG2 + 0.9999);
		free(dir);
		if(strncmp(uri,sf,prefix)==0 && strchr(sf+prefix,'/')==0 && sf[prefix]!=0)
			sf = sf+prefix;
		hprint(hout, "Fresh: %d,%s,%d,%s\r\n", pr, etag, siz, sf);
		newhint++;
continuei: ;
	}
	if(newhint)
		hprint(hout, "Fresh: have/%d\r\n", j);
}


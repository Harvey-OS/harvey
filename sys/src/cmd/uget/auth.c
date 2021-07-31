#include "mothra.h"

char base64[64]={
	"ABCDEFGH"
	"IJKLMNOP"
	"QRSTUVWX"
	"YZabcdef"
	"ghijklmn"
	"opqrstuv"
	"wxyz0123"
	"456789+/"
};
#define	PAD64	'='
void enc64(char *out, char *in)
{
	int b24;
	while(in[0]!='\0' && in[1]!='\0' && in[2]!='\0'){
		b24=((in[0]&255)<<16)|((in[1]&255)<<8)|(in[2]&255);
		*out++=base64[(b24>>18)&63];
		*out++=base64[(b24>>12)&63];
		*out++=base64[(b24>> 6)&63];
		*out++=base64[ b24     &63];
		in+=3;
	}
	if(in[0]!='\0'){
		b24=(in[0]&255)<<16;
		*out++=base64[(b24>>18)&63];
		if(in[1]=='\0'){
			*out++=base64[(b24>>12)&63];
			*out++=PAD64;
		}
		else{
			b24|=(in[1]&255)<<8;
			*out++=base64[(b24>>12)&63];
			*out++=base64[(b24>> 6)&63];
		}
		*out++=PAD64;
	}
	*out='\0';
}

static int
basicauth(char *arg, char *str, int n)
{
	int i;
	char *p;
	char buf[1024];
	Ibuf *b;

	if(strncmp(arg, "realm=", 6) == 0)
		arg += 6;
	if(*arg == '"'){
		arg++;
		for(p = arg; *p && *p != '"'; p++);
		*p = 0;
	} else {
		for(p = arg; *p && *p != ' ' && *p != '\t'; p++);
		*p = 0;
	}

	p = getenv("home");
	if(p == 0)
		p = getenv("HOME");
	if(p == 0){
		werrstr("$home not set");
		return -1;
	}
	snprint(buf, sizeof(buf), "%s/lib/mothra/insecurity", p);
	b = ibufopen(buf);
	if(b == 0){
		werrstr("www password file %s: %r", buf);
		return -1;
	}

	i = strlen(arg);
	while(p = rdline(b))
		if(strncmp(arg, p, i) == 0 && p[i] == '\t')
			break;
	if(p == 0){
		ibufclose(b);
		werrstr("no pwd for `%s'", arg);
		return -1;
	}

	p[linelen(b)-1] = 0;
	for(p += i; *p == '\t'; p++)
		;
	if(strlen(p) > 3*sizeof(buf)/4){
		ibufclose(b);
		werrstr("password too long: %s", p);
		return -1;
	}
	enc64(buf, p);
	snprint(str, n, "Authorization: Basic %s\r\n", buf);
	return 0;
}

int
auth(Url *url, char *str, int n)
{
	if(cistrcmp(url->authtype, "basic") == 0)
		return basicauth(url->autharg, str, n);
	werrstr("unknown auth method %s", url->authtype);
	return -1;
}

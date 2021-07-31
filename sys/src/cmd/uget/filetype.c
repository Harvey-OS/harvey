#include "mothra.h"

typedef struct Kind Kind;
struct Kind{
	char *name;
	int kind;
};
int
klook(char *s, Kind *k)
{
	while(k->name && cistrcmp(k->name, s)!=0)
		k++;
	return k->kind;
}
Kind suffix[]={
	".html",	HTML,
	".htm",		HTML,
	"/",		HTML,
	".gif",		GIF,
	".jpe",		JPEG,
	".jpg",		JPEG,
	".jpeg",	JPEG,
	".pic",		PIC,
	".au",		AUDIO,
	".tif",		TIFF,
	".tiff",	TIFF,
	".xbm",		XBM,
	".txt",		PLAIN,
	".text",	PLAIN,
	".ai",		POSTSCRIPT,
	".eps",		POSTSCRIPT,
	".pdf",		PDF,
	".ps",		POSTSCRIPT,
	0,		PLAIN
};
int
suflook(char *s, int len, Kind *k)
{
	int l;
	while(k->name){
		l=strlen(k->name);
		if(l<=len && cistrcmp(k->name, s+len-l)==0) return k->kind;
		k++;
	}
	return k->kind;
}
int
suffix2type(char *name)
{
	int len, kind, restore;
	char *s;
	len=strlen(name);
	if(len>=2 && cistrcmp(name+len-2, ".Z")==0){
		kind=COMPRESS;
		len-=2;
	}
	else if(len>=3 && cistrcmp(name+len-3, ".gz")==0){
		kind=GUNZIP;
		len-=3;
	}
	else
		kind=0;
	restore=name[len];
	name[len]='\0';
	for(s=name+len;s!=name && *s!='.';--s);
	kind|=suflook(name, len, suffix);
	name[len]=restore;
	return kind;
}
Kind content[]={
	"text/html",			HTML,
	"text/x-html",			HTML,
	"application/html",		HTML,
	"application/x-html",		HTML,
	"text/plain",			PLAIN,
	"image/gif",			GIF,
	"image/jpeg",			JPEG,
	"image/tiff",			TIFF,
	"image/x-xbitmap",		XBM,
	"image/x-bitmap",		XBM,
	"image/xbitmap",		XBM,
	"application/postscript",	POSTSCRIPT,
	0,				HTML
};
int
content2type(char *s)
{
	return klook(s, content);
}
Kind encoding[]={
	"x-compress",	COMPRESS,
	"compress",	COMPRESS,
	"x-gzip",	GUNZIP,
	"gzip",		GUNZIP,
	0,		0
};
int
encoding2type(char *s)
{
	return klook(s, encoding);
}
char*
type2content(int type)
{
	Kind *k;

	for(k = content; k->name; k++){
		if(k->kind == type)
			return k->name;
	}
	return "text/html";
}

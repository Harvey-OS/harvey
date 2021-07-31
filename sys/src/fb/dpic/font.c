/*
 * Typesetter font tables routines - for postprocessors.
 */
#include "ext.h"
Fonthd *fmount[MAXFONTS+1];		/* mount table - pointers into fonts[] */
Fonthd fonts[MAXFONTS+2];		/* font data - guarantee one empty slot */
int fcount;				/* entries in fonts[] */
int mcount;				/* fonts currently in memory */
int mlimit=MAXFONTS+1;			/* and the most we'll allow */
char *chnames[MAXCH];			/* special character hash table */
int nchnames;				/* number of entries in chnames[] */
/*
 * Check to see if the given DESC file is usable
 */
int checkdesc(char *path){
	return 1;	/* Wrong? */
}
/*
 * Read a typesetter description file. Font and size lists are discarded.
 */
int getdesc(char *path){
	char buf[150];
	FILE *fp;
	int n;
	if((fp=fopen(path, "r"))==0) return -1;
	while(fscanf(fp, "%s", buf)!=EOF){
		if(strcmp(buf, "res")==0)		fscanf(fp, "%d", &devres);
		else if(strcmp(buf, "unitwidth")==0)	fscanf(fp, "%d", &unitwidth);
		else if(strcmp(buf, "inmemory")==0)	fscanf(fp, "%d", &mlimit);
		else if(strcmp(buf, "charset")==0){
			while(fscanf(fp, "%s", buf)!=EOF)
				chadd(buf);
			break;
		}
		else if(strcmp(buf, "sizes")==0 )
			while(fscanf(fp, "%d", &n)!=EOF && n!=0);
		else if(strcmp(buf, "fonts")==0){
			fscanf(fp, "%d", &nfonts);
			for(n=0;n<nfonts;n++)
				fscanf(fp, "%s", buf);
		}
		skipline(fp);
	}
	fclose(fp);
	return 1;
}
/*
 * Read a width table from path into *fpos. Skip unnamed characters, spacewidth,
 * ligatures, ascender/descender entries, and anything else not recognized. All
 * calls should come through mountfont().
 */
int getfont(char *path, Fonthd *fpos){
	FILE *fin;
	Chwid chtemp[MAXCH];
	static Chwid chinit;
	int i, nw, n, wid, code;
	char buf[300], ch[100], s1[100], s2[100], s3[100], cmd[100];
	if(fpos->state==INMEMORY) return 1;
	if((fin=fopen(path, "r"))==0) return -1;
	if(fpos->state==NEWFONT){
		if(++fcount>MAXFONTS+1) return -1;
		fpos->path=strdup(path);
	}
	if(++mcount>mlimit && mcount>nfonts+1)
		freefonts();
	for(i=0;i<ALPHABET;i++)
		chtemp[i]=chinit;
	while(fscanf(fin, "%s", cmd)!=EOF){
		if(strcmp(cmd, "name")==0){
			release(fpos->name);
			fscanf(fin, "%s", buf);
			fpos->name = strdup(buf);
		}
		else if(strcmp(cmd, "fontname")==0){
			release(fpos->fontname);
			fscanf(fin, "%s", buf);
			fpos->fontname = strdup(buf);
		}
		else if(strcmp(cmd, "special")==0)
			fpos->specfont=1;
		else if(strcmp(cmd, "charset")==0){
			skipline(fin);
			nw=ALPHABET;
			while(fgets(buf, sizeof(buf), fin)!=0){
				sscanf(buf, "%s %s %s %s", ch, s1, s2, s3);
				if(s1[0]!='"'){		/* not a synonym */
					sscanf(s1, "%d", &wid);
					code=strtol(s3, 0, 0);	/* dec/oct/hex */
				}
				if(strlen(ch)==1){	/* it's ascii */
					n=ch[0];	/* origin omits non-graphics */
					chtemp[n].num=ch[0];
					chtemp[n].wid=wid;
					chtemp[n].code=code;
				}
				else if(ch[0]=='\\' && ch[1]=='0'){
					n=strtol(ch+1, 0, 0);/* check against ALPHABET? */
					chtemp[n].num=n;
					chtemp[n].wid=wid;
					chtemp[n].code=code;
				}
				else if(strcmp(ch, "---")!=0){	/* ignore unnamed chars */
					if((n=chindex(ch))==-1)	/* global? */
						n=chadd(ch);
					chtemp[nw].num=n;
					chtemp[nw].wid=wid;
					chtemp[nw].code=code;
					nw++;
				}
			}
			break;
		} 
		skipline(fin);
	}
	fclose(fin);
	fpos->wp=emalloc(nw*sizeof(Chwid));
	for(i=0;i<nw;i++)
		fpos->wp[i]=chtemp[i];
	fpos->nchars=nw;
	fpos->state=INMEMORY;
	return 1;
}
/*
 * Mount font table from file path at position m. Mounted fonts are guaranteed
 * to be in memory.
 */
int mountfont(char *path, int m){
	Fonthd *fpos;
	if(m<0 || m>MAXFONTS) return -1;
	if(fmount[m]!=0){
		if(strcmp(path, fmount[m]->path)==0){
			if(fmount[m]->state==INMEMORY)
				return 1;
		}
		else{
			fmount[m]->mounted--;
			fmount[m]=0;
		}
	}
	fmount[m]=fpos=&fonts[findfont(path)];
	fmount[m]->mounted++;
	return getfont(path, fpos);
}
/*
 * Release memory used by all unmounted fonts - except for the path string.
 */
void freefonts(void){
	int n;
	for(n=0;n<MAXFONTS+2;n++)
		if(fonts[n].state==INMEMORY && fonts[n].mounted==0){
			release(fonts[n].wp);
			fonts[n].wp=0;
			fonts[n].state=RELEASED;
			mcount--;
		}
}
/*
 * Look for path in the fonts table. Returns the index where it was found or can
 * be inserted (if not found).
 */
int findfont(char *path){
	int n;
	for(n=hash(path, MAXFONTS+2);fonts[n].state!=NEWFONT;n=(n+1)%(MAXFONTS+2))
		if(strcmp(path, fonts[n].path)==0)
			break;
	return n;
}
/*
 * Return 1 if a font is mounted at position m.
 */
int mounted(int m){
	return m>=0 && m<=MAXFONTS && fmount[m]!=0;
} 
/*
 * Returns the position of character c in the font mounted at m, or -1 if the
 * character is not found.
 */
int onfont(int c, int m){
	register Fonthd *fp;
	register Chwid *cp, *ep;
	if(mounted(m)){
		fp=fmount[m];
		if(c<ALPHABET){
			if(fp->wp[c].num==c)	/* ascii at front */
				return c;
			else return -1;
		}
		cp=&fp->wp[ALPHABET];
		ep=&fp->wp[fp->nchars];
		for(;cp<ep;cp++)			/* search others */
			if(cp->num==c)
				return cp - &fp->wp[0];
	}
	return -1;
}
/*
 * Look for s in global character name table. Hash table is guaranteed to have
 * at least one empty slot (by chadd()) so the loop terminate.
 */
int chindex(char *s){
	int i;
	for(i=hash(s, MAXCH);chnames[i]!=0;i=(i+1)%MAXCH)
		if(strcmp(s, chnames[i])==0)
			return i+ALPHABET;
	return -1;
}
/*
 * Add s to the global character name table. Leave one empty slot so loops
 * terminate.
 */
int chadd(char *s){
	int i;
	if(nchnames>=MAXCH-1)
		error("out of table space adding character %s", s);
	for(i=hash(s, MAXCH);chnames[i]!=0;i=(i+1)%MAXCH);
	nchnames++;
	chnames[i]=strdup(s);
	return i+ALPHABET;
}
/*
 * Returns string for the character with index n.
 */
char *chname(int n){
	return(chnames[n-ALPHABET]);
}
/*
 * Return the hash index for string s in a table of length l. Probably should
 * make i unsigned and mod once at the end.
 */
int hash(char *s, int l){
	register i;
	for(i=0;*s!='\0';s++)
		i=(i*10+(*s&255))%l;
	return i;
}
/*
 * Free memory provided ptr isn't 0.
 */
void release(void *ptr){
	if(ptr!=0) free(ptr);
}
/*
 * Dumps the font mounted at position n.
 */
void dumpmount(int m){
	if(fmount[m]!=0)
		dumpfont((fmount[m]-&fonts[0]));
	else fprintf(stderr, "no font mounted at %d\n", m);
}
/*
 * Dump of everything known about the font saved at fonts[n].
 */
void dumpfont(int n){
	int i;
	Fonthd *fpos;
	char *str;
	fpos=&fonts[n];
	if(fpos->state){
		fprintf(stderr, "path %s\n", fpos->path);
		fprintf(stderr, "state %d\n", fpos->state);
		fprintf(stderr, "mounted %d\n", fpos->mounted);
		fprintf(stderr, "nchars %d\n", fpos->nchars);
		fprintf(stderr, "special %d\n", fpos->specfont);
		fprintf(stderr, "name %s\n", fpos->name);
		fprintf(stderr, "fontname %s\n", fpos->fontname);
		if(fpos->state==INMEMORY){
			fprintf(stderr, "charset\n");
			for(i=0;i<fpos->nchars;i++){
				if(fpos->wp[i].num>0){
					if(fpos->wp[i].num<ALPHABET)
						fprintf(stderr, "%c\t%d\t%d\n",
							fpos->wp[i].num,
							fpos->wp[i].wid, fpos->wp[i].code);
					else{
						str=chname(fpos->wp[i].num);
						if(*str=='#'
						&& isdigit(*(str+1)) && isdigit(*(str+2)))
							str="---";
						fprintf(stderr, "%s\t%d\t%d\n",
							str, fpos->wp[i].wid,
							fpos->wp[i].code);
					}
				}
			}
		}
		else fprintf(stderr, "charset: not in memory\n");
	}
	else fprintf(stderr, "empty font: %d\n", n);
	putc('\n', stderr);
}

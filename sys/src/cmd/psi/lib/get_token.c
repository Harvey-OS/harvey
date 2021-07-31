#include <u.h>
#include <libc.h>
#include "system.h"
# include "stdio.h"
# include "defines.h"
# include "class.h"
# include "object.h"
#include <ctype.h>


struct object
get_token(FILE *fp, int *brace)
{
	char buf[STRING_LIMIT];
	register char *b=buf;
	char dbuf[STRING_LIMIT], *db;
	struct object object;
	int length, n, radix;
	int		c, class, dot=0, exp=0, paren, octal=0, pound=0, ibrace;
	enum	token	tok, token;
	char		*cp, *bend= &buf[STRING_LIMIT-2], *p;
	int (*fun)(FILE *);

	if(decryptf)fun=df_getc;
	else fun=f_getc;
getmore:
	c=fun(fp) ;
	if ( c == EOF )
		return(none);		/*tok=TK_EOF;*/
	else if ( c < 0  ||  c > 127 ){
		fprintf(stderr,"gettoken setting error %o\n",c);
		pserror("syntaxerror badchar", "get_token") ;
		/*tok=TK_ERROR1;*/
	}
	else class = map[c] ;
	switch(class){
	case ERROR:
		fprintf(stderr,"c= %o ", c);
		pserror("illegal character", "get_token");
	case LBRK:
		return(lbrak);
	case RBRK:
		return(rbrak);
	case LBRC:
		*brace = 1;
		return(lbrace);
	case RBRC:
		*brace = 2;
		return(rbrace);
	case WS:
		goto getmore;
	case PERC:
		*b++=c;
		while((c=fun(fp))){
			if(c == '\n' || c == '\r')break;
			*b++ = c;
		}
		if(buf[1] != '%'){
			b = buf;
			goto getmore;
		}
		*b = '\0';
		if(!strncmp("%%Page:",buf,7)){
			b=buf+7;
			cp = pagestr;
			if(*b == ' ')
				while(*b == ' ')b++;
			while(*b != ' ')
				*cp++ = *b++;
			*cp = '\0';
			pageno = atoi(pagestr);
			if(skipflag == 1){	/*tex doesn't have EndPage*/
rew:				skipflag=0;
				if((!reversed&&pageno>=page)||(reversed&&pageno<=page)){
					fseek(currentfile, prologend, 0);
					Line_no = prologline;
					pageno = 0;
				}
				while(1){
					object=get_token(currentfile, &ibrace);
					if(pageno == page){
						return(object);
					}
					if(object.type==OB_NONE){
						fprintf(stderr,"saw EOF while looking for page %d - last page is %d\n",page,pageno);
						pserror("eof", "get_token");
					}
				}
			}
			else if(skipflag && pageno==page)skipflag=0;
			if(pageflag == 2)tok = TK_EOF;
			if(!prolog){
				prolog = 1;
				prologend = (long)0;
				prologline = Line_no;
			}
		}
		else if(!strncmp("%%EndProlog",buf,13)){
			prolog = 1;
			prologend = Char_ct;
			prologline = Line_no;
		}
		else if(!strncmp("%%Trailer",buf,9))
			if(skipflag)goto rew;
		else if(!strncmp("%%EndPage:",buf,10)){
			if(skipflag)skipflag=2;
		}
		else if(!strncmp("%%Title: Microsoft Word",buf,23))
			microsoft = 1;
		b=buf;
		goto getmore;
	case LPAR:
		paren=1;
		while(1){
			if((*b=c=fun(fp)) == '(')paren++;
			else if(c == ')'){
				if(!(--paren))break;
			}
			else if(c == '\\'){
				c=fun(fp);
				switch(c){
				case '\n':continue;
				case 'b':*b = '\b';
					break;
				case 'f':*b = '\f';
					break;
				case 'n':*b = '\n';
					break;
				case 'r':*b = '\r';
					break;
				case 't':*b = '\t';
					break;
				default:
					if(isdigit(c)){
						octal=c-'0';
						if(isdigit(c=fun(fp))){
							octal=(octal*8+c-'0')&0xff;
							if(isdigit(c=fun(fp)))
							octal=(octal*8+c-'0')&0xff;
							else un_getc(c,fp);
						}
						else un_getc(c,fp);
						*b=octal;
					}
					else *b=c;
				}
			}
			if(b++ >= bend){
				fprintf(stderr,"leng %d\n",b-buf);
				fprintf(stderr,"buf %s\n",buf);
				pserror("limitcheck", "get_token LPAR");
			}
		}
				/*tok = TK_STRING;*/
		length = b-buf;
		object = makestring(length) ;
		memmove(object.value.v_string.chars,buf,length) ;
		return(object);
	case LT:
		while(1){
			if(isxdigit(c=fun(fp)))*b++ = c;
			else if(isspace(c))continue;
			else if(c == '>'){
				/*tok=TK_HEXSTRING;*/
				length = b-buf;
				object = makestring(length/2) ;
				for ( n=0 ; n<length ; n+=2 )
					object.value.v_string.chars[n/2] = hexcvt(buf[n])<<4 | hexcvt(buf[n+1]) ;
				return(object);
				break;
			}
			else{
				fprintf(stderr,"gettoken in LT setting ERROR %c\n",c);
				pserror("syntaxerror badchar", "get_token") ;
				/*tok=TK_ERROR1;*/
				break;
			}
			if(b >= bend){
				fprintf(stderr,"overrunning buffer in hexstring\n");
				fprintf(stderr,"buf %s\n",buf);
				pserror("limitcheck", "get_token LT");
			}
		}
		break;
	case SLASH:
		tok=TK_LITNAME;
		while(!isspace(c=fun(fp))){
			if((int)map[c] > 6){
				if(c == '/' && b == buf){
					tok=TK_IMMNAME;
					continue;
				}
				un_getc(c,fp);
				break;
			}
			*b++ = c;
		}
		length = b-buf;
		if(tok == TK_LITNAME){
			if(length==1 && isalpha(*buf)){
				if(*buf <= 'Z')return(lalpha[*buf-'A']);
				else return(lalpha[*buf-'a'+26]);
			}
			else return(makename((unsigned char *)buf,length,XA_LITERAL)) ;
		}
		else {
			return(dictstackget(makename((unsigned char *)buf,length,XA_LITERAL),
				"undefined immediate name")) ;
		}
	default:
		*b++ = c;
		tok=TK_NAME;
		if(class == DOT){
			tok=TK_REAL;
			dot=1;
		}
		else if(class == NUMBER|| class == SIGN)tok=TK_REAL;
		while(!isspace(c=fun(fp)) && (int)map[c&0177] < 7){
			*b++ = c;
			if(b >= bend){
				fprintf(stderr,"overrunning buffer in name\n");
				pserror("limitcheck", "get_token def");
			}
			if(tok == TK_NAME)continue;
			if(isdigit(c)||(pound&& isxdigit(c)))continue;
			switch(map[c]){
			case EXP:dot=1;
				exp=1;
				continue;
			case SIGN:
				if(!exp)tok=TK_NAME;
				continue;
			case DOT: if(dot)tok=TK_NAME;
				dot++;
				continue;
			case POUND: if(pound)tok=TK_NAME;
				pound++;
				continue;
			default:
				tok=TK_NAME;
			}
		}
		if(!isspace(c))un_getc(c,fp);
		*b = 0;
		length = b-buf;
		if(tok == TK_REAL){
			if(pound){		/*tok=TK_RADIXNUM;*/
				radix = atoi(buf) ;
				if ( 2 <= radix  &&  radix <= 36 ){
					n = strtol(strchr(buf,'#')+1,&p,radix) ;
					if ( *p == '\0' )
						return(makeint(n)) ;
					else return(makename((unsigned char *)buf,length,XA_EXECUTABLE)) ;
				}
				else return(makename((unsigned char *)buf,length,XA_EXECUTABLE)) ;
			}
			else if(!dot)	/*tok=TK_INTEGER;*/
				return(makeint(atoi(buf))) ;
			else return(makereal(atof(buf)));
		}
		if(tok == TK_NAME){
			if(length==1 && isalpha(*buf)){
				if(*buf <= 'Z')return(xalpha[*buf-'A']);
				else return(xalpha[*buf-'a'+26]);
			}
			else return(makename((unsigned char *)buf,length,XA_EXECUTABLE)) ;
		}
	}
}
void
skiptopage(void){
	struct object object;
	int brace;

	if((!reversed && pageno >= page) || (reversed && pageno<=page)){
		fseek(currentfile, prologend, 0);
		Line_no = prologline;
		pageno = 0;
	}
	while(1){
		object= get_token(currentfile, &brace);
		if(pageno == page)
			break;
		if(object.type == OB_NONE){
			fprintf(stderr,"saw EOF while looking for page %d - last page is %d\n",page,pageno);
			pserror("eof", "skiptopage");
		}
	}
	saveobj = object;
	saveflag=1;
}

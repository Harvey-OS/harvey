#include <u.h>
#include <libc.h>
# include <ctype.h>
#include "system.h"
# include "stdio.h"
# include "object.h"
# include "operator.h"
# include "defines.h"
# include "class.h"
#include "dict.h"

void
equalsOP(void)
{
	int		length, i ;
	unsigned char	*p ;

	p = cvs(pop(),&length) ;
	for ( i=0 ; i<length ;  i++ )
		fputc(p[i],stderr) ;
	fputc('\n',stderr) ;
}

void
equalsequalsOP(void)
{
	
	eqeq(pop(),0) ;
}

void
eqeq(struct object object, int level)
{
	int	i ;

	fprintf(stderr,"%*s",4*level,"") ;
	switch(object.type){
	case OB_BOOLEAN :
		fprintf(stderr,"%s",object.value.v_boolean==TRUE?"true":"false") ;
		break ;

	case OB_DICTIONARY :
		fprintf(stderr,"-dicttype-") ;
		break ;

	case OB_INT :
		fprintf(stderr,"%d",object.value.v_int) ;
		break ;

	case OB_MARK :
		fprintf(stderr,"-marktype-") ;
		break ;

	case OB_NAME :
		if ( object.xattr == XA_LITERAL )
			fprintf(stderr,"/") ;
		fprintf(stderr,"%.*s",object.value.v_string.length,object.value.v_string.chars) ;
		break ;

	case OB_STRING :
		fprintf(stderr,"(%.*s)",object.value.v_string.length,object.value.v_string.chars) ;
		break ;

	case OB_REAL :
		fprintf(stderr,"%.1lf",object.value.v_real) ;
		break ;

	case OB_ARRAY :
		if ( object.xattr == XA_EXECUTABLE )
			fprintf(stderr,"{\n") ;
		else
			fprintf(stderr,"[\n") ;
		for ( i=0 ; i<object.value.v_array.length ; i++ )
			eqeq(object.value.v_array.object[i],level+1) ;
		fprintf(stderr,"%*s",4*level,"") ;
		if ( object.xattr == XA_EXECUTABLE )
			fprintf(stderr,"}") ;
		else
			fprintf(stderr,"]") ;
		break ;

	case OB_FILE :
		fprintf(stderr,"-filetype-") ;
		break ;

	case OB_FONTID :
		fprintf(stderr,"-fontidtype-") ;
		break ;

	case OB_NULL :
		fprintf(stderr,"-nulltype-") ;
		break ;

	case OB_OPERATOR :
		for ( i=0 ; i<Op_nel ; i++ )
			if ( object.value.v_operator == (void(*)(void))Op_tab[i].v_op )
				break ;
		if ( i == Op_nel )
			fprintf(stderr,"--%x--",object.value.v_operator) ;
		else
			fprintf(stderr,"--%s--",Nop_tab[i].chars) ;
		break ;

	case OB_SAVE :
		fprintf(stderr,"-savetype-") ;
		break ;

	default :
		fprintf(stderr,"type %d\n",object.type);
		error("bad case in equalsequals") ;
	}
	fprintf(stderr,"\n") ;
}

void
fileOP(void)
{
	FILE		*fp ;
	char		buf[1024] ;
	struct	object	access;

	access = pop() ;
	if ( access.type != OB_STRING ){
		fprintf(stderr,"file access not string\n",access.type);
		pserror("typecheck", "file") ;
	}
	filename = pop() ;
	if ( filename.type != OB_STRING ){
		fprintf(stderr,"filename not string\n",access.type);
		pserror("typecheck", "file") ;
	}
	if ( access.value.v_string.length != 1 ){
		pserror("invalidfileaccess", "file") ;
	}
	if ( filename.value.v_string.length + 1 > NEL(buf) ){
		fprintf(stderr,"file filename too long %d\n",
			filename.value.v_string.length);
		pserror("limitcheck", "file") ;
	}
	memmove(buf,filename.value.v_string.chars,filename.value.v_string.length) ;
	buf[filename.value.v_string.length] = '\0' ;
	switch(access.value.v_string.chars[0]){
	case 'r' :
		fp = fopen(buf,"r") ;
		if ( fp == NULL ){
				fprintf(stderr,"name %s ",buf);
				pserror("undefinedfilename", "file read") ;
		}
		push(makefile(fp,AC_READONLY,XA_LITERAL)) ;
		return;

	case 'w' :
		fp = fopen(buf,"r+") ;
		if ( fp == NULL ){
				fp = fopen(buf,"w+") ;
				if ( fp == NULL ){
					fprintf(stderr,"name %s ",buf);
					pserror("undefinedfilename", "file write") ;
				}
		}
		push(makefile(fp,AC_UNLIMITED,XA_LITERAL)) ;
		return;

	default :
		pserror("invalidfileaccess", "file") ;
	}
}

void
flushfileOP(void)
{
	struct	object	file ;
	int i;

	file = pop() ;
	if ( file.type != OB_FILE )
		pserror("typecheck", "flushfile") ;
	if ( file.value.v_file.access == AC_READONLY ){
		if ( (i=fseek(file.value.v_file.fp,0,2)) != 0 ){
			fprintf(stderr,"i %d\n",i);
			pserror("ioerror", "flushfile1") ;
		}
	}
	else if ( fflush(file.value.v_file.fp) == EOF )
			pserror("ioerror", "flushfile2") ;
}

void
closefileOP(void)
{
	struct	object	file ;
	file = pop() ;
	push(file) ;
	flushfileOP() ;
	if(decryptf){
		decryptf=0;
		return;
	}
	if ( fclose(file.value.v_file.fp) == EOF )
		pserror("ioerror", "closefile") ;
}

void
readOP(void)
{
	int		c ;
	struct	object	file ;

	file = pop() ;
	if ( file.type != OB_FILE ){
		fprintf(stderr,"type %o\n",file.type);
		pserror("typecheck", "read") ;
	}
	if ( file.value.v_file.access == AC_NONE)
		pserror("invalidaccess", "read") ;
	if(decryptf)pserror("decrypt", "read");
	c = fgetc(file.value.v_file.fp) ;
	if ( c == EOF )
		if ( ferror(file.value.v_file.fp) )
			pserror("ioerror", "read") ;
		else
			push(false) ;
	else {
		push(makeint(c)) ;
		push(true) ;
	}
}

void
writeOP(void)
{
	int		c ;
	struct	object	file, object ;
	object = integer("write");
	c = object.value.v_int ;
	file = pop() ;
	if ( file.type != OB_FILE )
		pserror("typecheck", "write") ;
	if ( file.value.v_file.access != AC_UNLIMITED )
		pserror("invalidaccess", "write") ;
	if ( fputc(c,file.value.v_file.fp) == EOF )
		pserror("ioerror", "write") ;
}

void
readlineOP(void)
{
	int		i, c ;
	struct	object	string, file ;

	string = pop() ;
	if ( string.type != OB_STRING )
		pserror("typecheck", "readline") ;
	file = pop() ;
	if ( file.type != OB_FILE )
		pserror("typecheck", "readline") ;
	fseek(file.value.v_file.fp,0,1) ;
	if(decryptf)pserror("decrypt", "readline");
	for ( i=0 ; i<string.value.v_string.length ; i++ )
		switch ( c=fgetc(file.value.v_file.fp) ){
		case '\n' :
			string.value.v_string.length = i ;
			push(string) ;
			push(true) ;
			return ;

		case EOF :
			string.value.v_string.length = i ;
			push(string) ;
			push(false) ;
			return ;

		default :
			SAVEITEM(string.value.v_string.chars[i]) ;
			string.value.v_string.chars[i] = c ;
			break ;
		}
	pserror("rangecheck", "readline") ;
}

void
readstringOP(void)
{
	int		i, c ;
	struct	object	filled, string, file ;
	int (*fun)(FILE *);

	if(decryptf)fun=df_getc;
	else fun=f_getc;

	string = pop() ;
	if ( string.type != OB_STRING )
		pserror("typecheck", "readstring") ;
	file = pop() ;
	if ( file.type != OB_FILE )
		pserror("typecheck", "readstring") ;
	/*fseek(file.value.v_file.fp,0,0) ;*/
	filled = true ;
	/*if(decryptf)fprintf(stderr,"readstringOP decrypt leng %d\n",string.value.v_string.length);*/
	for ( i=0 ; i<string.value.v_string.length ; i++ )
		if ( (c=fun(file.value.v_file.fp)) == EOF ){
			filled = false ;
			break ;
		}
		else{
			SAVEITEM(string.value.v_string.chars[i]) ;
			string.value.v_string.chars[i] = c ;
		}
	string.value.v_string.length = i ;
	push(string) ;
	push(filled) ;
}

void
readhexstringOP(void)
{
	int		x1, x2, i ;
	struct object	filled, string, file ;
	string = pop() ;
	if ( string.type != OB_STRING )
		pserror("typecheck", "readhexstring") ;
	file = pop() ;
	if ( file.type != OB_FILE )
		pserror("typecheck", "readhexstring") ;
	fseek(file.value.v_file.fp,0,1) ;
	filled = true ;
	if(decryptf)pserror("decrypt", "readhexstring");
	for ( i=0 ; i<string.value.v_string.length ; i++ ){
		x1 = fgetx(file.value.v_file.fp) ;
		if ( x1 == -1 ){
			filled = false ;
			break ;
		}
		SAVEITEM(string.value.v_string.chars[i]) ;
		string.value.v_string.chars[i] = x1<<4 ;
		x2 = fgetx(file.value.v_file.fp) ;
		if ( x2 == -1 ){
			filled = false ;
			break ;
		}
		string.value.v_string.chars[i] |= x2 ;
	}
	string.value.v_string.length = i ;
	push(string) ;
	push(filled) ;
}

int
fgetx(FILE *fp)
{
	int	c ;

	while ( (c=fgetc(fp)) != EOF ){
		if(c == '\n')Line_no++;
		if ( isdigit(c) )
			return(c-'0') ;
		else if ( isxdigit(c) )
			if ( isupper(c) )
				return(c-'A'+10) ;
			else
				return(c-'a'+10) ;
	}
	return(-1) ;
}

void
writestringOP(void)
{
	int		i ;
	struct	object	string, file ;

	string = pop() ;
	if ( string.type != OB_STRING )
		pserror("typecheck", "writestring") ;
	file = pop() ;
	if ( file.type != OB_FILE )
		pserror("typecheck", "writestring") ;
	fseek(file.value.v_file.fp,0,1) ;
	for ( i=0 ; i<string.value.v_string.length ; i++ )
		if ( fputc(string.value.v_string.chars[i],file.value.v_file.fp) == EOF)
			pserror("ioerror", "writestring") ;
}

void
writehexstringOP(void)
{
	int		i ;
	struct	object	string, file ;

	string = pop() ;
	if ( string.type != OB_STRING )
		pserror("typecheck", "writehexstring") ;
	file = pop() ;
	if ( file.type != OB_FILE )
		pserror("typecheck", "writehexstring") ;
	fseek(file.value.v_file.fp,0,1) ;
	for ( i=0 ; i<string.value.v_string.length ; i++ )
		if(fprintf(file.value.v_file.fp,"%02x",string.value.v_string.chars[i])
		     != 2 )
			pserror("ioerror", "writehexstring") ;
}

void
bytesavailableOP(void)
{
	int		count ;
	struct	object	file ;

	file = pop() ;
	if ( file.type != OB_FILE )
		pserror("typecheck", "bytesavailable") ;
	if ( feof(file.value.v_file.fp) )
		count = -1 ;
	else
		count = file.value.v_file.fp->wp - file.value.v_file.fp->rp ;
	push(makeint(count)) ;
}

void
statusOP(void)
{
	struct	object	status, file ;

	file = pop() ;
	if ( file.type != OB_FILE )
		pserror("typecheck", "status") ;
	if ( ferror(file.value.v_file.fp) != 0 )
		status = false ;
	else
		status = true ;
	push(status) ;
}

void
tokenOP(void)
{
	int		length ;
	struct	object	object, source ;
	FILE		*fp ;

	source = pop() ;
	switch(source.type){
	case OB_FILE :
		object = get_token(source.value.v_file.fp,&length) ;
		if ( object.type == OB_NONE ){
			push(source) ;
			closefileOP() ;
			push(false) ;
		}
		else {
			push(object) ;
			push(true) ;
		}
		return;

	case OB_STRING :
		fp = sopen(source.value.v_string) ;
		object = scanner(fp) ;
		source.value.v_string.chars += fp->rp - fp->buf ;	/*plan9*/
		source.value.v_string.length -= fp->rp - fp->buf ;	/*plan9*/
		f_close(fp);
		if ( object.type == OB_NONE )
			push(false) ;
		else {
			push(source) ;
			push(object) ;
			push(true) ;
		}
		return;

	default :
		pserror("typecheck", "token") ;
	}
}

void
runOP(void)
{
	char		buf[BUFSIZ] ;
	FILE		*fp ;

	filename = pop() ;
	if ( filename.type != OB_STRING )
		pserror("typecheck", "run") ;
	if ( filename.value.v_string.length + 1 > NEL(buf) )
		pserror("limitcheck", "run") ;
	memmove(buf,filename.value.v_string.chars,filename.value.v_string.length) ;
	buf[filename.value.v_string.length] = '\0' ;
	fp = fopen(buf,"r") ;
	if ( fp == NULL ){
			fprintf(stderr,"name %s ",buf);
			pserror("undefinedfilename", "run") ;
	}
	currentfile = fp;
	execpush(makefile(fp,AC_EXECUTEONLY,XA_EXECUTABLE)) ;
}

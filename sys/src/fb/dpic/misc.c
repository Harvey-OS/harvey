/*
 * General purpose routines.
 */
#include "ext.h"
int	nolist=0;			/* number of specified ranges */
int	olist[50];			/* processing range pairs */
/*
 * Grab page ranges from str, save them in olist[], and update the nolist
 * count. Range syntax matches nroff/troff syntax.
 */
void out_list(char *str){
	int start, stop;
	while(*str && nolist<sizeof(olist)-2){
		start=stop=str_convert(&str, 0);
		if(*str=='-' && *str++)
			stop=str_convert(&str, 9999);
		if(start>stop)
			error("illegal range %d-%d", start, stop);
		olist[nolist++]=start;
		olist[nolist++]=stop;
		if(*str!='\0') str++;
	}
	olist[nolist]=0;

}
/*
 * Return true if num is in the current page range list. Print everything if
 * there's no list.
 */
int in_olist(int num){
	int i;
	if(nolist==0)
		return 1;
	for(i=0;i<nolist;i+=2)
		if(num>=olist[i] && num<=olist[i+1])
			return 1;
	return 0;
}
/*
 * Grab the next integer from **str and return its value or err if *str
 * isn't an integer. *str is modified after each digit is read.
 */
int str_convert(char **str, int err){
	int i;
	if(!isdigit(**str))
		return err;
	for(i=0;isdigit(**str);*str+=1)
		i=10*i+**str-'0';
	return i;
}
/*
 * Print an error message and quit.
 */
void error(char *mesg, ...){
	va_list vlist;
	if(mesg!=0 && *mesg!='\0'){
		fprintf(stderr, "%s: ", prog_name);
		va_start(vlist, mesg);
		vfprintf(stderr, mesg, vlist);
		va_end(vlist);
		if(lineno>0)
			fprintf(stderr, " (line %d)", lineno);
		putc('\n', stderr);
	}
	if(!ignore)
		exits("fatal");
}
void *emalloc(int n){
	void *v=malloc(n);
	if(v==0) error("no space");
	return v;
}

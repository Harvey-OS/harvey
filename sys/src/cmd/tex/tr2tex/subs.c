/* COPYRIGHT (C) 1987 Kamal Al-Yahya */
/* 
These subroutines do (in general) small things for the translator.
They appear in alphabetical order and their names are unique in the
first six characters.
*/

#include        "setups.h"
#include        "simil.h"
#include        "greek.h"
#include        "flip.h"
#include        "forbid.h"
#include        "maths.h"
#include        "macros.h"

extern def_count;
extern mydef_count;

/* compile-time counting of elements */
int GRK_count = (sizeof(GRK_list)/sizeof(GRK_list[0]));
int sim_count = (sizeof(sim_list)/sizeof(sim_list[0]));
int flip_count = (sizeof(flip_list)/sizeof(flip_list[0]));
int forbd_count = (sizeof(forbid)/sizeof(forbid[0]));
int mathcom_count = (sizeof(math)/sizeof(struct math_equiv));
int macro_count = (sizeof(macro)/sizeof(struct macro_table));

char *
alternate(char *inbuf, char *outbuf, char *w)		/* alternate fonts (manual macro) */
{
int f1,f2;
int which=1;
char font[MAXWORD], font1[MAXWORD], font2[MAXWORD],
     ww[MAXWORD], tmp[MAXWORD];

tmp[0] = NULL;
f1 = w[1];	f2 = w[2];
if (f1 == 'R')	strcpy(font1,"\\rm");
if (f1 == 'I')	strcpy(font1,"\\it");
if (f1 == 'B')	strcpy(font1,"\\bf");
if (f2 == 'R')	strcpy(font2,"\\rm");
if (f2 == 'I')	strcpy(font2,"\\it");
if (f2 == 'B')	strcpy(font2,"\\bf");

strcpy(font,font1);
while (*inbuf != '\n' && *inbuf != NULL)
	{
	inbuf += get_arg(inbuf,ww,1);
	sprintf(tmp,"{%s %s}",font,ww);
	outbuf = strapp(outbuf,tmp);
	if (which == 1)
		{
		which = 2;
		strcpy(font,font2);
		}
	else
		{
		which = 1;
		strcpy(font,font1);
		}
	while (*inbuf == ' ' || *inbuf == '\t')
		inbuf++;
	}

return(outbuf);
}

int
CAP_GREEK(char *w)			/* check if w is in the GREEK list */
{
int i;

for (i=0; i < GRK_count ; i++)
	{
	if (strcmp(GRK_list[i],w) == 0)
		return(1);
	}
return(-1);
}

char *
do_table(char *inbuf, char *outbuf, int *offset)
{
char w[MAXWORD], ww[MAXWORD], format[MAXWORD], tmp[MAXWORD];
char *ptr;
int i,j,len,columns=0;
int tab = '\t';				/* default tab */

tmp[0] = NULL;
ptr = inbuf;				/* remember where we started */
len = get_line(inbuf,w,0);
if (w[strlen(w)-1] == ';')		/* options */
	{
	inbuf += len;
	if (strncmp(w,"tab",3) == 0)	/* get the tab charecter */
		tab = w[4];		/* expect something like tab(&); */
	inbuf = skip_line(inbuf);
	}
while (*inbuf != NULL)			/* get the LAST format line */
	{
	len = get_line(inbuf,w,0);
	if (w[strlen(w)-1] != '.')	break;	/* not a fromat line */
	inbuf += len;
	for (i=0, j=0; i<len-1; i++)
		{
		if (isspace(w[i]))	continue;
		columns++;
		if (w[i] == 'l')	format[j] = 'l';
		else if (w[i] == 'r')	format[j] = 'r';
		else			format[j] = 'c';
		j++;
		}
	}
if (columns == 0)
	{
	fprintf(stderr,"Sorry, I cannot do tables without a format line\nDoing plain translation of table, lines will be commented\nYou need to fix it yourself\n");
	while (*inbuf != NULL)
		{
		(void) getword(inbuf,w);
		if (strcmp(w,".TE") ==  0)	{inbuf += 4;	break;}
		inbuf += get_line(inbuf,w,1);
		*outbuf++ = '%';
		outbuf = strapp(outbuf,w);
		outbuf = strapp(outbuf,"\n");
		inbuf++;		/* skip the \n */
		}
	*offset = inbuf - ptr;
	return(outbuf);
	}
format[j] = NULL;
sprintf(tmp,"\\par\n\\begin{tabular}{%s}\n",format);
outbuf = strapp(outbuf,tmp);

while (*inbuf != NULL)
	{
	for (i=0; i<columns-1; i++)
		{
		(void) getword(inbuf,w);
		if (i == 0 && (strcmp(w,"\n") == 0 || strcmp(w,"_") == 0))
			{inbuf++;	i--;	continue;}
		if (strcmp(w,".TE") == 0)
			{
			inbuf += 4;
			if (i == 0)
				{
				outbuf -= 3;	/* take back the \\ and the \n */
				*outbuf = NULL;
				}
			outbuf = strapp(outbuf,"\n\\end{tabular}\n\\par\n");
			*offset = inbuf - ptr;
			return(outbuf);
			}
		inbuf += get_table_entry(inbuf,w,tab);
		inbuf ++;		/* skip tab */
		troff_tex(w,ww,0,1);
		sprintf(tmp,"%s & ",ww);
		outbuf = strapp(outbuf,tmp);
		}
	(void) getword(inbuf,w);
	if (strcmp(w,".TE") == 0)
		{
		fprintf(stderr,"Oops! I goofed. I told I you I am not very good at tables\nI've encountered an unexpected end for the table\nYou need to fix it yourself\n");
		inbuf += 4;
		outbuf = strapp(outbuf,"\\end{tabular}\n\\par\n");
		*offset = inbuf - ptr;
		return(outbuf);
		}
	inbuf += get_table_entry(inbuf,w,'\n');
	inbuf++;		/* skip tab */
	troff_tex(w,ww,0,1);
	outbuf = strapp(outbuf,ww);
	outbuf = strapp(outbuf,"\\\\\n");
	}
fprintf(stderr,"Oops! I goofed. I told I you I am not very good at tables\nFile ended and I haven't finished the table!\nYou need to fix it yourself\n");
*offset = inbuf - ptr;
outbuf = strapp(outbuf,"\\end{tabular}\n\\par\n");
return(outbuf);
}

char *
end_env(char *outbuf)
{
if (IP_stat)
	{
	IP_stat = 0;
	outbuf = strapp(outbuf,"\\end{itemize}");
	}
if (QP_stat)
	{
	QP_stat = 0;
	outbuf = strapp(outbuf,"\\end{quotation}");
	}
if (TP_stat)
	{
	TP_stat = 0;
	outbuf = strapp(outbuf,"\\end{TPlist}");
	}
return(outbuf);
}

void
envoke_stat(int par)
{

switch(par)
	{
	case 2:
		IP_stat = 1;
		break;
	case 3:
		TP_stat = 1;
		break;
	case 4:
		QP_stat = 1;
		break;
	default:
		break;
	}
}

char *
flip(char *outbuf, char *w)			/* do the flipping */
{
int lb=0, rb=0;
char ww[MAXWORD], tmp[MAXWORD];

ww[0] = NULL;	tmp[0] = NULL;
outbuf--;
while (*outbuf == ' ' || *outbuf == '\t' || *outbuf == '\n')
	outbuf--;
while (1)
	{
	if (*outbuf == '{')
		{
		lb++;
		if (lb > rb)	break;
		}
	if (*outbuf == '}')	rb++;
	if (rb == 0)
		{
		if (*outbuf != ' ' && *outbuf != '\t' && *outbuf != '\n'
			&& *outbuf != '$')
			{
			outbuf--;
			continue;
			}
		else	break;
		}
	outbuf--;
	if (lb == rb && lb != 0)	break;
	}
outbuf++;
if (*outbuf == '\\')
	{
	outbuf++;
	(void) getword(outbuf,tmp);
	sprintf(ww,"\\%s",tmp);
	outbuf--;
	}
else if (*outbuf == '{')
	(void) get_brace_arg(outbuf,ww);
else
	(void) getword(outbuf,ww);
*outbuf = NULL;
sprintf(tmp,"\\%s %s",w,ww);
outbuf = strapp(outbuf,tmp);
return(outbuf);
}

char *
flip_twice(char *outbuf, char *w, char *ww)		/* take care of things like x hat under */
{
int lb=0, rb=0;
char tmp1[MAXWORD], tmp2[MAXWORD];

tmp1[0] = NULL;		tmp2[0] = NULL;
outbuf--;
while (*outbuf == ' ' || *outbuf == '\t' || *outbuf == '\n')
	outbuf--;
while (1)
	{
	if (*outbuf == '{')
		{
		lb++;
		if (lb > rb)	break;
		}
	if (*outbuf == '}')	rb++;
	if (rb == 0)
		{
		if (*outbuf != ' ' && *outbuf != '\t' && *outbuf != '\n'
			&& *outbuf != '$')
			{
			outbuf--;
			continue;
			}
		else	break;
		}
	outbuf--;
	if (lb == rb && lb != 0)	break;
	}
outbuf++;
if (*outbuf == '\\')
	{
	outbuf++;
	(void) getword(outbuf,tmp2);
	sprintf(tmp1,"\\%s",tmp2);
	outbuf--;
	}
else if (*outbuf == '{')
	(void) get_brace_arg(outbuf,tmp1);
else
	(void) getword(outbuf,tmp1);
*outbuf = NULL;
sprintf(tmp2,"\\%s{\\%s %s}",w,ww,tmp1);
outbuf = strapp(outbuf,tmp2);
return(outbuf);
}

int
get_arg(char *inbuf, char *w, int rec)		/* get argument; rec == 1 means recursive */
{
int c,len,i;
char ww[MAXWORD];
int delim;

len=0;
while ((c = *inbuf) == ' ' || c == '\t')	/* skip spaces and tabs */
		{inbuf++;	len++;}
i=0;
if (*inbuf == '{' || *inbuf == '\"')
	{
	if (*inbuf == '{')	delim = '}';
	else			delim = '\"';
	inbuf++;	len++;
	while ((c = *inbuf++) != NULL && c != delim && i < MAXWORD)
		{
		if (c == ' ' && delim == '\"')	ww[i++] = '\\';
		ww[i++] = (char)c;	len++;
		}
	len++;
	}
else
	{
	while ((c = *inbuf++) != NULL && c != ' ' && c != '\t' && c != '\n'
		&& c != '$' && c != '}' && i < MAXWORD)
		{
		if (math_mode && c == '~')	break;
		ww[i++] = (char)c;	len++;
		}
	}
ww[i] = NULL;
if (rec == 1)				/* check if recursion is rquired */
	troff_tex(ww,w,1,1);
else
	strcpy(w,ww);
return(len);
}

void
get_brace_arg(char *buf, char *w)		/* get argumnet surrounded by braces */
{
int c,i, lb=0, rb=0;

i=0;
while ((c = *buf++) != NULL)
	{
	w[i++] = (char)c;
	if (c == '{')	lb++;
	if (c == '}')	rb++;
	if (lb == rb)	break;
	}
w[i] = NULL;
}

int
get_defword(char *inbuf, char *w, int *illegal)		/* get "define" or .de word */
{
int c,i;

*illegal = 0;
for (i=0; (c = *inbuf++) != NULL && c != ' ' && c != '\n'
		&& c != '\t' && i < MAXWORD; i++)
	{
	w[i] = (char)c;
	if (isalpha(c) == 0)	*illegal = 1;	/* illegal TeX macro */ 
	}
w[i] = NULL;
if (*illegal == 0)
	if (is_forbid(w) >= 0)		*illegal=1;
return(i);
}

int
get_line(char *inbuf, char *w, int rec)		/* get the rest of the line */
{
int c,i,len;
char ww[MAXLINE];

i=0;	len=0;
while ((c = *inbuf++) != NULL && c != '\n' && len < MAXLINE)
		{ww[i++] = (char)c;	len++;}
ww[i] = NULL;
if (rec == 1)
	troff_tex(ww,w,0,1);
else
	strcpy(w,ww);
return(len);
}

int
get_multi_line(char *inbuf, char *w)		/* get multi-line argument */
{
int len=0,l=0,lines=0;
char tmp[MAXWORD];
int c1,c2;

w[0] = NULL;	tmp[0] = NULL;
while (*inbuf != NULL)
	{
	c1 = *inbuf;	c2 = *++inbuf;		--inbuf;
	if (c1 == '.' && isupper(c2))		break; 
	lines++;
	if (lines > 1)
		strcat(w," \\\\\n");
	l = get_line(inbuf,tmp,1);
	strcat(w,tmp);
	len += l+1;	inbuf += l+1;
	}
len--;		inbuf--;
return(len);
}

int
get_mydef(char *inbuf, char *w)		/* get the macro substitution */
{
int c1,c2,l,len;
char tmp[MAXWORD];

tmp[0] = NULL;
len=1;
while (*inbuf != NULL)
	{
	c1 = *inbuf;	c2 = *++inbuf;		--inbuf;
	if (c1 == '.' && c2 == '.')		break; 
	l = get_line(inbuf,tmp,1);
	strcat(w,tmp);
	len += l+1;	inbuf += l+1;
	}
return(len);
}
int
get_N_lines(char *inbuf, char *w, int N)		/* get N lines */
{
int len=0,l=0,lines=0;
char tmp[MAXWORD];

w[0] = NULL;	tmp[0] = NULL;
while (*inbuf != NULL && lines < N)
	{
	lines++;
	if (lines > 1)
		strcat(w," \\\\\n");
	l = get_line(inbuf,tmp,1);
	strcat(w,tmp);
	len += l+1;	inbuf += l+1;
	}
len--;		inbuf--;
return(len);
}

int
get_no_math(char *inbuf, char *w)		/* get text surrounded by quotes in math mode */
{
int c,i,len;

len = 0;
for (i=0; (c = *inbuf++) != NULL && c != '\"' && i < MAXWORD; i++)
	{
	if (c == '{' || c == '}')
		{w[i] = '\\';	w[++i] = (char)c;}
	else
		w[i] = (char)c;
	len++;
	}
w[i] = NULL;
return(len);
}

char *
get_over_arg(char *inbuf, char *ww)		/* get the denominator of over */
{
char w[MAXWORD], tmp1[MAXWORD], tmp2[MAXWORD];
int len;

w[0] = NULL;	tmp1[0] = NULL;		tmp2[0] = NULL;
inbuf += getword(inbuf,tmp1);		/* read first word */
inbuf += skip_white(inbuf);
len = getword(inbuf,tmp2);		/* read second word */
strcat(w,tmp1);	strcat(w," ");

/* as long as there is a sup or sub read the next two words */
while (strcmp(tmp2,"sub") == 0 || strcmp(tmp2,"sup") == 0)
	{
	inbuf += len;
	strcat(w,tmp2); strcat(w," ");
	inbuf += skip_white(inbuf);
	inbuf += getword(inbuf,tmp1);
	strcat(w,tmp1); strcat(w," ");
	inbuf += skip_white(inbuf);
	len = getword(inbuf,tmp2);
	}
troff_tex(w,ww,0,1);
return(inbuf);
}

int
get_ref(char *inbuf, char *w)		/* get reference */
{
int len=0, l=0, lines=0;
char tmp[MAXWORD];

w[0] = NULL;	tmp[0] = NULL;
while (*inbuf != NULL)
	{
	if (*inbuf == '\n')		break;
	(void) getword(inbuf,tmp);
	if (tmp[0] == '.' && isupper(tmp[1]))
		{
/* these commands don't cause a break in reference */
		if (strcmp(tmp,".R") != 0 && strcmp(tmp,".I") != 0
			&& strcmp(tmp,".B") != 0)
			break; 
		}
	else if (tmp[0] == '.' && !(isupper(tmp[1])))
		{
/* these commands don't cause a break in reference */
		if (strcmp(tmp,".br") != 0 && strcmp(tmp,".bp") != 0)
			break; 
		}
	l = get_line(inbuf,tmp,1);
	lines++;
	if (lines > 1)		strcat(w," ");
	strcat(w,tmp);
	len += l+1;	inbuf += l+1;
	}
len--;		inbuf--;
return(len);
}

void
get_size(char *ww, struct measure *PARAMETER)
{
int sign=0, units=0;
float value;

if (ww[0] == NULL)
	{
	if (PARAMETER->def_value == 0)
		{
		PARAMETER->value = PARAMETER->old_value;
		strcpy(PARAMETER->units,PARAMETER->old_units);
		}
	else
		{
		PARAMETER->value = PARAMETER->def_value;
		strcpy(PARAMETER->units,PARAMETER->def_units);
		}
	}
else
	{
	PARAMETER->old_value = PARAMETER->value;
	strcpy(PARAMETER->old_units,PARAMETER->units);
	parse_units(ww,&sign,&value,&units);
	if (units == 'p')
		strcpy(PARAMETER->units,"pt");
	else if (units == 'i')
		strcpy(PARAMETER->units,"in");
	else if (units == 'c')
		strcpy(PARAMETER->units,"cm");
	else if (units == 'm')
		strcpy(PARAMETER->units,"em");
	else if (units == 'n')
		{
		value = .5*value;	/* n is about half the width of m */
		strcpy(PARAMETER->units,"em");
		}
	else if (units == 'v')
		strcpy(PARAMETER->units,"ex");
	else if (units == 0)
		{
		if (sign == 0 || PARAMETER->old_units[0] == NULL)
			strcpy(PARAMETER->units,PARAMETER->def_units);
		else
			strcpy(PARAMETER->units,PARAMETER->old_units);
		}
	else
		{
		fprintf(stderr,"unknown units %c, using default units\n");
		strcpy(PARAMETER->units,PARAMETER->def_units);
		}
	if (sign == 0)	PARAMETER->value = value;
	else		PARAMETER->value = PARAMETER->old_value + sign*value;
	}
}

int
get_string(char *inbuf, char *w, int rec)	/* get the rest of the line -- Nelson Beebe */
{
int c,i,len;
char ww[MAXLINE];
char *start;

if (*inbuf != '\"')
    return(get_line(inbuf,w,rec));
start = inbuf;				/* remember start so we can find len */
i=0;
inbuf++;				/* point past initial quote */
while ((c = *inbuf++) != NULL && c != '\"' && c != '\n' && i < MAXLINE)
    ww[i++] = (char)c;
ww[i] = NULL;
if (c != '\n')				/* flush remainder of line */
    while ((c = *inbuf++) != '\n')
	/* NO-OP */;
len = inbuf - start - 1;		/* count only up to NL, not past */
if (rec == 1)
	troff_tex(ww,w,0,1);
else
	strcpy(w,ww);
return(len);
}

int
get_sub_arg(char *inbuf, char *w)		/* get the argument for sub and sup */
{
int c,len,i;
char ww[MAXWORD], tmp[MAXWORD];

len=0;	tmp[0] = NULL;
while ((c = *inbuf) == ' ' || c == '\t')
		{inbuf++;	len++;}
i=0;
while ((c = *inbuf++) != NULL && c != ' ' && c != '\t' && c != '\n'
		&& c != '$' && c != '}' && c != '~' && i < MAXWORD)
		{ww[i++] = (char)c;	len++;}
ww[i] = NULL;
if (strcmp(ww,"roman") == 0  || strcmp(ww,"bold") == 0 || strcmp(w,"italic") == 0)
	{
	(void) get_arg(inbuf,tmp,0);
	sprintf(ww,"%s%c%s",ww,c,tmp);
	len += strlen(tmp)+1;
	}
troff_tex(ww,w,0,1);		/* recursive */
return(len);
}

int
get_table_entry(char *inbuf, char *w, int tab)
{
int c, i=0;

for (i=0; (c = *inbuf++) != NULL && c != tab && i < MAXWORD; i++)
		w[i] = (char)c;
w[i] = NULL;

return(i);
}

int
get_till_space(char *inbuf, char *w)			/* get characters till the next space */
{
int c,i;

for (i=0; (c = *inbuf++) != NULL && c != ' ' && c != '\n'
		&& c != '\t' && i < MAXWORD; i++)
	w[i] = (char)c;
w[i] = NULL;
return(i);
}

int
getdef(char *inbuf, char *ww)		/* get the define substitution */
{
int c,i,len;
int def_delim;
char w[MAXWORD];

def_delim = *inbuf++;		/* take first character as delimiter */
len=1;		i=0;
while ((c = *inbuf++) != NULL && c != def_delim && i < MAXWORD)
	{len++;		w[i++] = (char)c;}
w[i] = NULL;
len++;
if (c != def_delim)
	{
	fprintf(stderr,"WARNING: missing right delimiter in define, define=%s\n",w);
	len--;
	}
troff_tex(w,ww,0,1);		/* now translate the substitution */
return(len);
}

int
getword(char *inbuf, char *w)		/* get an alphanumeric word (dot also) */
{
int c,i;

for (i=0; (c = *inbuf++) != NULL
	&& (isalpha(c) || isdigit(c) || c == '.') && i < MAXWORD; i++)
		w[i] = (char)c;
if (i == 0 && c != NULL)
	w[i++] = (char)c;
w[i] = NULL;
return(i);
}

void
GR_to_Greek(char *w, char *ww)			/* change GREEK to Greek */
{
*ww++ = '\\';		*ww++ = *w;
while(*++w != NULL)
	*ww++ = tolower(*w);
*ww = NULL;
}

int
is_def(char *w)		/* check if w was defined by the user */
{
int i;

for (i=0; i < def_count; i++)
	{
	if (strcmp(def[i].def_macro,w) == 0)
		return(i);
	}
return(-1);
}

int
is_flip(char *w)		/* check if w is in the flip list */
{
int i;

for (i=0; i < flip_count; i++)
	{
	if (strcmp(flip_list[i],w) == 0)
		return(i);
	}
return(-1);
}

int
is_forbid(char *w)		/* check if w is one of those sacred macros */
{
int i;

for (i=0; i < forbd_count; i++)
	{
	if (strcmp(forbid[i],w) == 0)
		return(i);
	}
return(-1);
}

int
is_mathcom(char *w, char *ww)	/* check if w has a simple correspondence in TeX */
{
int i;

for (i=0; i < mathcom_count; i++)
	{
	if (strcmp(math[i].troff_symb,w) == 0)
		{
		strcpy(ww,math[i].tex_symb);
		return(i);
		}
	}
return(-1);
}

int
is_mydef(char *w)		/* check if w is user-defined macro */
{
int i;

for (i=0; i < mydef_count; i++)
	{
	if (strcmp(mydef[i].def_macro,w) == 0)
		return(i);
	}
return(-1);
}

int
is_troff_mac(char *w, char *ww, int *arg, int *par)/* check if w is a macro or plain troff command */
{
int i;

for (i=0; i < macro_count; i++)
	{
	if (strcmp(macro[i].troff_mac,w) == 0)
		{
		strcpy(ww,macro[i].tex_mac);
		*arg = macro[i].arg;
		*par = macro[i].macpar;
		return(i);
		}
	}
return(-1);
}

void
parse_units(char *ww, int *sign, float *value, int *units)
{
int len, k=0, i;
char tmp[MAXWORD];

len = strlen(ww);
if (ww[0] == '-')	*sign = -1;
else if (ww[0] == '+')	*sign = 1;
if (*sign != 0)		k++;

i=0;
while (k < len)
	{
	if (isdigit(ww[k]) || ww[k] == '.')
		tmp[i++] = ww[k++];
	else	break;
	}
tmp[i] = NULL;
sscanf(tmp,"%f",value);
i=0;
if (k < len)
	{
	*units = ww[k++];
	if (k < len)
		fprintf(stderr,
		"Suspect problem in parsing %s, unit used is %c\n",ww,*units);
	}
}

void
scrbuf(FILE *in, FILE *out)			/* copy input to output */
{
int c;
while ((c =getc(in)) != EOF)	putc(c,out);
}

int
similar(char *w)			/* check if w is in the similar list */
{
int i;

for (i=0; i < sim_count ; i++)
	{
	if (strcmp(sim_list[i],w) == 0)
		return(1);
	}
return(-1);
}

char *
skip_line(char *inbuf)		/* ignore the rest of the line */
{
while (*inbuf != '\n' && *inbuf != NULL)
	inbuf++;
if (*inbuf == NULL)	return(inbuf);
else			return(++inbuf);
}

int
skip_white(char *inbuf)		/* skip white space */
{
int c,len=0;

while ((c = *inbuf++) == ' ' || c == '\t' || c == '\n')
	len++;	
return(len);
}

char *
strapp(char *s, char *tail)  /* copy tail[] to s[], return ptr to terminal NULL in s[] */
{
while (*s++ = *tail++)
	/*NO-OP*/;
return (s-1);			/* pointer to NULL at end of s[] */
}

void
tmpbuf(FILE *in, char *buffer)
/* copy input to buffer, buffer holds only MAXLEN characters */
{
int c;
unsigned int l=0;

while (l++ < MAXLEN && (c = getc(in)) != EOF)
	*buffer++ = (char)c;
if (l >= MAXLEN)
	{
	fprintf(stderr,"Sorry: document is too large\n");
	exit(-1);
	}
*buffer = NULL;
}

/* COPYRIGHT (C) 1987 Kamal Al-Yahya */

/*
This program has the HARD-WIRED rules of the translator.
It should handled with care.
*/

#define IN_TR  1
#include        "setups.h"
int def_count = 0;
int mydef_count = 0;

void
troff_tex(char *inbuf, char *outbuf, int mid, int rec)
{
char eqn_no[MAXWORD], w[MAXWORD], ww[MAXLINE], tmp[MAXWORD], tmp2[MAXWORD];
char *p;
int len,c,c1,c2,i,j;
int ref = 0;
int put_brace = 0;
int first_word = 1;
int no_word = 1;
int arg = 0;
int par = 0;
int illegal = 0;
int floating = 0;
static int delim_defd = 0;	/* whether math delimiter has been defined */
static char *DELIM = "$";
float flen;
int N;
int RSRE = 0;			/* block indentation */
int thisfont = 1;		/* default font is roman */
int lastfont = 1;		/* default last font is roman */
int offset = 0;			/* amount to offset inbuf */
extern man;			/* man flag */

*outbuf = NULL;		w[0] = NULL;	ww[0] = NULL;
tmp[0] = NULL;		tmp2[0] = NULL;
while (*inbuf != NULL)
	{
	len = getword(inbuf,w);
	c1 = *--inbuf;
	c2 = *++inbuf;
	inbuf += len;
	if (isspace(w[0]) == 0)		no_word = 0;
/* first check if we are in math mode */
	if (math_mode)
		{
		len = get_till_space(inbuf,ww);
		sprintf(tmp,"%s%s",w,ww);
		if (strcmp(w,"delim") == 0)
			{
			delim_defd = 1;
			inbuf += skip_white(inbuf);
			DELIM[0] = *inbuf;
			inbuf = skip_line(inbuf);
			}
/* check if it is a math delimiter; switch to non-math mode if so */
		else if (delim_defd && strcmp(w,DELIM) == 0)
			{
			math_mode = 0;
			*outbuf++ = '$';
			}
/* check for illegal macros here */
		else if (len > 0 && def_count > 0 && (i=is_def(tmp)) >= 0)
			{
			inbuf += len;
			outbuf = strapp(outbuf,def[i].replace);
			}
/* See if it is a (legally) defined macro */
		else if (def_count > 0 && (i=is_def(w)) >= 0)
			{
			if (def[i].illegal)
				outbuf = strapp(outbuf,def[i].replace);
			else
				{
				outbuf = strapp(outbuf,"\\");
				outbuf = strapp(outbuf,w);
				}
			}
/* Search for commands in some order; start with non-alphanumeric symbols */
		else if (strcmp(w,"#") == 0 || strcmp(w,"&") == 0
			 || strcmp(w,"%") == 0 || strcmp(w,"_") == 0)
			{
			outbuf = strapp(outbuf,"\\");
			outbuf = strapp(outbuf,w);
			}
		else if (strcmp(w,"=") == 0)
			{
			if (*inbuf == '=')
				{
				inbuf++;
				outbuf = strapp(outbuf,"\\equiv");
				}
			else
				outbuf = strapp(outbuf,"=");
			}
		else if (strcmp(w,"<") == 0 || strcmp(w,">") == 0)
			{
			if (*inbuf == '=')
				{
				inbuf++;
				if (strcmp(w,"<") == 0)
					outbuf = strapp(outbuf,"\\le");
				else
					outbuf = strapp(outbuf,"\\ge");
				}
			}
		else if (strcmp(w,"-") == 0)
			{
			if (*inbuf == '>')
				{
				inbuf++;
				outbuf = strapp(outbuf,"\\to");
				}
			else if (*inbuf == '+')
				{
				inbuf++;
				outbuf = strapp(outbuf,"\\mp");
				}
			else
				*outbuf++ = '-';
			}
		else if (strcmp(w,"+") == 0)
			{
			if (*inbuf == '-')
				{
				inbuf++;
				outbuf = strapp(outbuf,"\\pm");
				}
			else
				*outbuf++ = '+';
			}
		else if (strcmp(w,"\"") == 0)
			{
			len = get_no_math(inbuf,ww);
			inbuf += len+1;
			if (len > 1)
				{
				sprintf(tmp,"\\ \\it\\hbox{%s}",ww);
				outbuf = strapp(outbuf,tmp);
				}
			else if (len == 1)
				*outbuf++ = ww[0];
			}
/* Now search for symbols that start with a captial */
		else if (strcmp(w,".EN") == 0)
			{
			math_mode = 0;
			if ((len=strlen(eqn_no)) > 0)
				{
				sprintf(tmp,"\\eqno %s",eqn_no);
				outbuf = strapp(outbuf,tmp);
				}
			eqn_no[0] = NULL;
			c1 = *--outbuf;
			c2 = *--outbuf;
			if (c1 == '\n' && c2 == '$')
				*--outbuf = NULL;
			else
				{
				outbuf += 2;
				outbuf = strapp(outbuf,"$$");
				}
			}
/* Now search for symbols that start with a small letter */
		else if (strcmp(w,"bold") == 0 || strcmp(w,"roman") == 0 ||
			 strcmp(w,"italic") == 0)
			{
			inbuf += get_arg(inbuf,ww,1);
			if (strcmp(w,"bold") == 0)
				{
				sprintf(tmp,"{\\bf %s}",ww);
				outbuf = strapp(outbuf,tmp);
				}
			else if (strcmp(w,"roman") == 0)
				{
				sprintf(tmp,"{\\rm %s}",ww);
				outbuf = strapp(outbuf,tmp);
				}
			else
				{
				sprintf(tmp,"{\\it %s}",ww);
				outbuf = strapp(outbuf,tmp);
				}
			}
		else if (strcmp(w,"define") == 0)
			{
			if (def_count >= MAXDEF)
				{
				fprintf(stderr,
					"Too many defines. MAXDEF=%d\n",MAXDEF);
				exit(-1);
				}
			for (i=0; *--outbuf != '$' && i < MAXLEN; i++)
				tmp[i] = *outbuf;
			tmp[i] = NULL;
			strcat(tmp,"$$");
			*--outbuf = NULL;
			inbuf += skip_white(inbuf);
			inbuf += get_defword(inbuf,w,&illegal);
			inbuf += skip_white(inbuf);
			inbuf += getdef(inbuf,ww);
			if (illegal)
				{
				def[def_count].illegal = 1;
				fprintf(stderr,
					"illegal TeX macro, %s, replacing it\n",w);
				p = (char *)malloc((unsigned)(strlen(ww)+1)*
					sizeof(char));
				strcpy(p,ww);
				def[def_count].replace = p;
				}
			else
				{
				def[def_count].illegal = 0;
				sprintf(tmp2,"\\def\\%s{%s}\n",w,ww);
				outbuf = strapp(outbuf,tmp2);
				}
			p = (char *)malloc((unsigned)(strlen(w)+1)*sizeof(char));
			strcpy(p,w);
			def[def_count++].def_macro = p;
			inbuf += skip_white(inbuf);
			for (j=i+1; j >= 0; j--)
				*outbuf++ = tmp[j];
			tmp[0] = NULL;
			}
		else if (strcmp(w,"gsize") == 0 || strcmp(w,"gfont") == 0)
			inbuf = skip_line(inbuf);
		else if (strcmp(w,"left") == 0 || strcmp(w,"right") == 0)
			{
			sprintf(tmp,"\\%s",w);
			outbuf = strapp(outbuf,tmp);
			inbuf += skip_white(inbuf);
			len = getword(inbuf,ww);
			if (strcmp(ww,"floor") == 0)
				{
				inbuf += len;
				if (strcmp(w,"left") == 0)
					outbuf = strapp(outbuf,"\\lfloor");
				else
					outbuf = strapp(outbuf,"\\rfloor");
				}
			else if (strcmp(ww,"nothing") == 0 || ww[0] == '\"')
				{
				inbuf += len;
				*outbuf++ = '.';
				if (ww[0] == '\"')	inbuf++;
				}
			else if (*inbuf == '{' || *inbuf == '}')
				*outbuf++ = '\\';
			}
		else if (strcmp(w,"over") == 0)
			{
			if (!first_word)
				{
				outbuf--;
				for (i=0; *outbuf == ' ' || *outbuf == '\t' ||
						*outbuf =='\n'; i++)
					tmp[i] = *outbuf--;
				if (*outbuf == '}' && put_brace == 0)
					*outbuf = ' ';
				else
					{
					for (; !(*outbuf == ' ' || *outbuf == '\t'
					|| *outbuf =='\n' || *outbuf == '$'); i++)
						tmp[i] = *outbuf--;
					put_brace = 0;
				*++outbuf = '{';
					}
				for (j=i-1; j >= 0; j--)
					*++outbuf = tmp[j];
			*++outbuf = NULL;
				}
			outbuf = strapp(outbuf,"\\over");
			inbuf += skip_white(inbuf);
			*outbuf++ = ' ';
			if (*inbuf == '{')
				inbuf++;
			else
				{
				inbuf = get_over_arg(inbuf,ww);
				outbuf = strapp(outbuf,ww);
				if (*inbuf != NULL || !first_word)
					*outbuf++ = '}';
				}
			}
		else if (strcmp(w,"size") == 0)
			inbuf += get_arg(inbuf,ww,0);
		else if (strcmp(w,"sup") == 0 || strcmp(w,"to") == 0 ||
			 strcmp(w,"sub") == 0 || strcmp(w,"from") == 0)
			{
			while ((c = *--outbuf) == ' ' || c == '\t' || c == '\n') ;
			*++outbuf = NULL;
			if (strcmp(w,"sup") == 0 || strcmp(w,"to") == 0)
				outbuf = strapp(outbuf,"^");
			else
				outbuf = strapp(outbuf,"_");
			inbuf += skip_white(inbuf);
			len = get_sub_arg(inbuf,ww);
			inbuf += len;
			if (len > 1)
				{
				sprintf(tmp,"{%s}",ww);
				outbuf = strapp(outbuf,tmp);
				len = skip_white(inbuf);
				inbuf += len;
				(void) getword(inbuf,ww);
				if (strcmp(ww,"over") == 0)
					put_brace = 1;
				inbuf -= len;
				}
			else
				outbuf = strapp(outbuf,ww);
			}
		else if (strcmp(w,"up") == 0 || strcmp(w,"down") == 0
			 || strcmp(w,"fwd") == 0 || strcmp(w,"back") == 0)
			{
			if (strcmp(w,"up") == 0)
				{
				outbuf = strapp(outbuf,"\\raise");
				strcpy(tmp,"ex");
				}
			else if (strcmp(w,"down") == 0)
				{
				outbuf = strapp(outbuf,"\\lower");
				strcpy(tmp,"ex");
				}
			else if (strcmp(w,"fwd") == 0)
				{
				outbuf = strapp(outbuf,"\\kern");
				strcpy(tmp,"em");
				}
			else if (strcmp(w,"back") == 0)
				{
				outbuf = strapp(outbuf,"\\kern-");
				strcpy(tmp,"em");
				}
			inbuf += skip_white(inbuf);
			inbuf += getword(inbuf,ww);
			len = atoi(ww);		flen = len/100.;
			ww[0] = NULL;
			sprintf(tmp2,"%4.2f%s",flen,tmp);
			outbuf = strapp(outbuf,tmp2);
			}
/* Now check if the word is a member of a group */
		else if (CAP_GREEK(w) > 0)
			{
			GR_to_Greek(w,ww);
			outbuf = strapp(outbuf,ww);
			}
		else if (is_flip(w) >= 0)
			{
			if (!first_word)
				{
				len = skip_white(inbuf);
				inbuf += len;
				(void) getword(inbuf,ww);
				if (is_flip(ww) >= 0)
					{
					inbuf += strlen(ww);
					outbuf = flip_twice(outbuf,w,ww);
					}
				else
					{
					inbuf -= len;
					outbuf = flip(outbuf,w);
					}
				}
			else
				{
				outbuf = strapp(outbuf,"\\");
				outbuf = strapp(outbuf,w);
				}
			}
		else if (is_mathcom(w,ww) >=0 )
			outbuf = strapp(outbuf,ww);
		else if (similar(w) > 0)
			{
			outbuf = strapp(outbuf,"\\");
			outbuf = strapp(outbuf,w);
			}

/* if none of the above math commands matched, it is an ordinary symbol;
   just copy it */

		else	outbuf = strapp(outbuf,w);
		}

/* check if it is a math delimiter; switch to math mode if so */

	else if (strcmp(w,"$") == 0 && de_arg > 0)
		{
		de_arg++;
		*outbuf++ = '#';
		}
	else if (delim_defd && strcmp(w,DELIM) == 0)
		{
		math_mode = 1;
		*outbuf++ = '$';
		}
	else if (strcmp(w,"$") == 0)
		outbuf = strapp(outbuf,"\\$");

/* check if it is a non-math troff command */

	else if ((c2 == '.') && !(mid) && (c1 == '\n' || (first_word)))
		{
/* Search in some order; start with non-alphanumeric characters */
		if (strcmp(w,".") == 0)
			{
			c1 = *inbuf;
			c2 = *++inbuf;
			if (c1 == '\\' && c2 == '\"')
				{
				++inbuf;
				inbuf += get_line(inbuf,ww,0);
				outbuf = strapp(outbuf,"%");
				outbuf = strapp(outbuf,ww);
				}
			else
				{
				fprintf(stderr,
				"I cannot translate troff macro .%c%c\n",c1,c2);
				inbuf += get_line(inbuf,ww,0);
				sprintf(tmp,"%%.%c%c",c1,c2);
				outbuf = strapp(outbuf,tmp);
				outbuf = strapp(outbuf,ww);
				if (*inbuf == NULL)	*outbuf++ = '\n';
				}
			}
/* Now search for commads that start with a capital */
		else if (strcmp(w,".AB") == 0)
			{
			inbuf += get_arg(inbuf,ww,0);
			if (strcmp(ww,"no") == 0)
				outbuf = strapp(outbuf,"\\bigskip");
			else
				outbuf = strapp(outbuf,"\\begin{abstract}");
			}
		else if (strcmp(w,".B") == 0 || strcmp(w,".bf") == 0 ||
			 strcmp(w,".I") == 0 || strcmp(w,".it") == 0 ||
			 strcmp(w,".R") == 0 || strcmp(w,".rm") == 0)
			{
			if (strcmp(w,".R") == 0 || strcmp(w,".rm") == 0)
				strcpy(w,"rm");
			else if (strcmp(w,".B") == 0 || strcmp(w,".bf") == 0)
				strcpy(w,"bf");
			else
				strcpy(w,"it");
			inbuf += get_arg(inbuf,ww,1);
			if (ww[0] == NULL)
				{
				outbuf = strapp(outbuf,"\\");
				outbuf = strapp(outbuf,w);
				}
			else
				{
				sprintf(tmp,"{\\%s %s}",w,ww);
				outbuf = strapp(outbuf,tmp);
				}
			}
		else if (man && (strcmp(w,".BR") == 0 || strcmp(w,".BI") == 0
			 || strcmp(w,".IR") == 0 || strcmp(w,".IB") == 0 ||
			 strcmp(w,".RI") == 0 || strcmp(w,".RB") == 0))
			{
			outbuf = alternate(inbuf,outbuf,w);
			inbuf = skip_line(inbuf);
			*outbuf++ = '\n';
			}
		else if (strcmp(w,".BX") == 0)
			{
			inbuf += get_arg(inbuf,ww,1);
			sprintf(tmp,"\\fbox{%s}",ww);
			outbuf = strapp(outbuf,tmp);
			}
		else if (strcmp(w,".EQ") == 0)
			{
			math_mode = 1;
			put_brace = 0;
			outbuf = strapp(outbuf,"$$");
			len = get_arg(inbuf,eqn_no,0);
			if (strcmp(eqn_no,"I") == 0 || strcmp(eqn_no,"L") == 0)
				{
				fprintf(stderr,"lineups are ignored\n");
				inbuf += len;
				len = get_arg(inbuf,eqn_no,0);
				}
			if ((strlen(eqn_no)) > 0)
				inbuf += len;
			len = get_arg(inbuf,tmp,0);
			if (strcmp(tmp,"I") == 0 || strcmp(tmp,"L") == 0)
				{
				fprintf(stderr,"lineups are ignored\n");
				inbuf += len;
				}
			}
		else if (strcmp(w,".IP") == 0)
			{
			inbuf += get_arg(inbuf,ww,1);
			inbuf = skip_line(inbuf);
			if (IP_stat == 0)
				outbuf = strapp(outbuf,"\\begin{itemize}\n");
			sprintf(tmp,"\\item[{%s}]\n",ww);
			outbuf = strapp(outbuf,tmp);
			if (de_arg > 0)		mydef[mydef_count].par = 2;
			else			IP_stat = 1;
			}
		else if (strcmp(w,".KE") == 0)
			{
			if (floating)
				outbuf = strapp(outbuf,"\\end{figure}");
			else
				outbuf = strapp(outbuf,"}");
			floating = 0;
			}
		else if (strcmp(w,".KF") == 0)
			{
			floating = 1;
			outbuf = strapp(outbuf,"\\begin{figure}");
			}
		else if (strcmp(w,".QP") == 0)
			{
			if (de_arg > 0)		mydef[mydef_count].par = 4;
			else			QP_stat = 1;
			outbuf = strapp(outbuf,"\\begin{quotation}");
			}
		else if (strcmp(w,".RE") == 0)
			{
			RSRE--;
			if (RSRE < 0)
				fprintf(stderr,".RS with no matching .RE\n");
			sprintf(tmp,"\\ind{%d\\parindent}",RSRE);
			outbuf = strapp(outbuf,tmp);
			}
		else if (strcmp(w,".RS") == 0)
			{
			RSRE++;
			sprintf(tmp,"\\ind{%d\\parindent}",RSRE);
			outbuf = strapp(outbuf,tmp);
			}
		else if (strcmp(w,".Re") == 0)
			{
			if (ref == 0)
				outbuf = strapp(outbuf,"\\REF\n");
			ref++;
			inbuf = skip_line(inbuf);
			inbuf += get_ref(inbuf,ww);
			sprintf(tmp,"\\reference{%s}",ww);
			outbuf = strapp(outbuf,tmp);
			}
		else if (man && (strcmp(w,".TP") == 0 || strcmp(w,".HP") == 0))
			{
			if (IP_stat && TP_stat)
				{
				outbuf = strapp(outbuf,"\\end{itemize}%\n");
				IP_stat = 0;
				}
			if (QP_stat && TP_stat)
				{
				outbuf = strapp(outbuf,"\\end{quotation}%\n");
				QP_stat = 0;
				}
			inbuf = skip_line(inbuf);
			inbuf += get_line(inbuf,ww,1);
			if (TP_stat == 0)
				{
				sprintf(tmp,"\\begin{TPlist}{%s}\n",ww);
				outbuf = strapp(outbuf,tmp);
				}
			sprintf(tmp,"\\item[{%s}]",ww);
			outbuf = strapp(outbuf,tmp);
			if (de_arg > 0)		mydef[mydef_count].par = 3;
			else			TP_stat = 1;
			}
		else if (man && (strcmp(w,".TH") == 0))
			{
/* expect something like .TH LS 1 "September 4, 1985"*/
			outbuf = strapp(outbuf,"\\phead");
			for (j = 1; j <= 3; ++j)
				{
				inbuf += get_arg(inbuf,ww,0);
				sprintf(tmp,"{%s}",ww);
				outbuf = strapp(outbuf,tmp);
				}
			*outbuf++ = '\n';
			}
		else if (strcmp(w,".TS") == 0)
			{
			fprintf(stderr,"I am not very good at tables\n\
I can only do very simple ones. You may need to check what I've done\n");
			inbuf = skip_line(inbuf);
			outbuf = do_table(inbuf,outbuf,&offset);
			inbuf += offset;
			offset = 0;		/* reset */
			}
/* Now search for commands that start with small letters */
		else if (strcmp(w,".TE") == 0)
			{
			fprintf(stderr,"Oops! I goofed. I told you I am not very good at tables.\nI have encountered a table end but I am not in table mode\n");
			}
		else if (strcmp(w,".de") == 0)
			{
			de_arg = 1;
			if (mydef_count >= MAXDEF)
				{
				fprintf(stderr,
					"Too many .de's. MAXDEF=%d\n",MAXDEF);
				exit(-1);
				}
			inbuf += skip_white(inbuf);
			inbuf += get_defword(inbuf,w,&illegal);
			inbuf += skip_white(inbuf);
			inbuf += get_mydef(inbuf,ww);
			mydef[mydef_count].arg_no = de_arg;
			if (illegal)
				{
				mydef[mydef_count].illegal = 1;
				fprintf(stderr,
					"illegal TeX macro, %s, replacing it\n",w);
				p = (char *)malloc((unsigned)(strlen(ww)+2)*
					sizeof(char));
				sprintf(p,"%s",ww);
				mydef[mydef_count].replace = p;
				}
			else
				{
				mydef[mydef_count].illegal = 0;
				sprintf(tmp,"\\def\\%s",w);
				outbuf = strapp(outbuf,tmp);
				for (j=1; j<de_arg; j++)
					{
					sprintf(tmp,"#%d",j);
					outbuf = strapp(outbuf,tmp);
					}
				sprintf(tmp,"{%s}\n",ww);
				outbuf = strapp(outbuf,tmp);
				}
			p = (char *)malloc((unsigned)(strlen(w)+2)*sizeof(char));
			sprintf(p,".%s",w);
			mydef[mydef_count++].def_macro = p;
			inbuf = skip_line(inbuf);
			de_arg = 0;
			}
		else if (strcmp(w,".ds") == 0)
			{
			inbuf += get_arg(inbuf,w,0);
			inbuf += skip_white(inbuf);
			inbuf += get_line(inbuf,ww,1);
			if (strcmp(w,"LH") == 0)
				{
				sprintf(tmp,"\\lefthead{%s}",ww);
				outbuf = strapp(outbuf,tmp);
				}
			else if (strcmp(w,"RH") == 0)
				{
				sprintf(tmp,"\\righthead{%s}",ww);
				outbuf = strapp(outbuf,tmp);
				}
			else if (strcmp(w,"CF") == 0)
				{
				if (strchr(ww,'%') == 0)
					{
					sprintf(tmp,"\\footer{%s}",ww);
					outbuf = strapp(outbuf,tmp);
					}
				else
					outbuf = strapp(outbuf,
							"\\footer{\\rm\\thepage}");
				}
			else
				{
				fprintf(stderr,"I do not understand .ds %s\n",w);
				sprintf(tmp,"%%.ds %s %s",w,ww);
				outbuf = strapp(outbuf,tmp);
				}
			}
		else if (strcmp(w,".sp") == 0)
			{
			inbuf += get_arg(inbuf,ww,0);
			(void) get_size(ww,&space);
			sprintf(tmp,"\\par\\vspace{%3.1f%s}",
				space.value,space.units);
			outbuf = strapp(outbuf,tmp);
			}
		else if (strcmp(w,".in") == 0)
			{
			inbuf += get_arg(inbuf,ww,0);
			(void) get_size(ww,&indent);
			sprintf(tmp,"\\ind{%3.1f%s}",indent.value,indent.units);
			outbuf = strapp(outbuf,tmp);
			}
		else if (strcmp(w,".ls") == 0)
			{
			inbuf += get_arg(inbuf,ww,0);
			(void) get_size(ww,&linespacing);
			sprintf(tmp,"\\baselineskip=%3.1f%s",linespacing.value,
					linespacing.units);
			outbuf = strapp(outbuf,tmp);
			}
		else if (strcmp(w,".so") == 0)
			{
			inbuf += get_arg(inbuf,ww,0);
			sprintf(tmp,"\\input %s",ww);
			outbuf = strapp(outbuf,tmp);
			}
		else if (strcmp(w,".ti") == 0)
			{
			inbuf += get_arg(inbuf,ww,0);
			tmpind.value = indent.value;
			strcpy(tmpind.units,indent.units);
			(void) get_size(ww,&tmpind);
			sprintf(tmp,"\\tmpind{%3.1f%s}",
				tmpind.value,tmpind.units);
			outbuf = strapp(outbuf,tmp);
			}
		else if (strcmp(w,".vs") == 0)
			{
			inbuf += get_arg(inbuf,ww,0);
			(void) get_size(ww,&vspace);
			sprintf(tmp,"\\par\\vspace{%3.1f%s}",
				vspace.value,vspace.units);
			outbuf = strapp(outbuf,tmp);
			}
/* check if it is a member of a group */
		else if (mydef_count > 0 && (i=is_mydef(w)) >= 0)
			{
			if (mydef[i].par > 0)
				{
				if (de_arg > 0)
					mydef[mydef_count].par = mydef[i].par;
				else
					{
					outbuf = end_env(outbuf);
					outbuf = strapp(outbuf,"\n");
					}
				}
			if (mydef[i].illegal)
				outbuf = strapp(outbuf,mydef[i].replace);
			else
				{
				w[0] = '\\';	/* replace dot by backslash */
				outbuf = strapp(outbuf,w);
				}
			for (j=1; j <mydef[i].arg_no; j++)
				{
				inbuf += get_arg(inbuf,ww,1);
				sprintf(tmp,"{%s}",ww);
				outbuf = strapp(outbuf,tmp);
				}
			if (de_arg == 0)	envoke_stat(mydef[i].par);
			}
		else if ((i=is_troff_mac(w,ww,&arg,&par)) >= 0)
			{
			if (par > 0)
				{
				if (de_arg > 0)	mydef[mydef_count].par = par;
				else		outbuf = end_env(outbuf);
				}
			outbuf = strapp(outbuf,ww);
			if (ww[0] == NULL)
				inbuf = skip_line(inbuf);
			if (ww[0] != NULL && arg == 0)
				{
				inbuf = skip_line(inbuf);
				*outbuf++ = '\n';
				}
			if (arg > 0) 
				{
				if (arg == 1)
					{
					inbuf += skip_white(inbuf);
					inbuf += get_string(inbuf,ww,1);
					}
				else
					{
					if (isupper(w[1]))
						{
						inbuf = skip_line(inbuf);
						inbuf += get_multi_line(inbuf,ww);
						}
					else
						{
						inbuf += get_arg(inbuf,tmp,0);
						inbuf = skip_line(inbuf);
						if (tmp[0] == NULL)	N = 1;
						else		N = atoi(tmp);
						inbuf += get_N_lines(inbuf,ww,N);
						}
					}
				sprintf(tmp2,"{%s}",ww);
				outbuf = strapp(outbuf,tmp2);
				}
			}
/* if none of the above commands matched, it is either
   an illegal macro or an unknown command */
		else
			{
			len = get_till_space(inbuf,ww);
			sprintf(tmp,"%s%s",w,ww);
			if (mydef_count > 0 && (i=is_mydef(tmp)) >= 0)
				{
				inbuf += len;
				if (mydef[i].par > 0)
					{
					if (de_arg > 0)
						mydef[mydef_count].par=mydef[i].par;
					else
						{
						outbuf = end_env(outbuf);
						outbuf = strapp(outbuf,"\n");
						}
					}
				outbuf = strapp(outbuf,mydef[i].replace);
				for (j=1; j <mydef[i].arg_no; j++)
					{
					inbuf += get_arg(inbuf,ww,1);
					sprintf(tmp,"{%s}",ww);
					outbuf = strapp(outbuf,tmp);
					}
				if (de_arg == 0)	envoke_stat(mydef[i].par);
				}
			else
				{
				fprintf(stderr,
				"I cannot translate troff macro %s\n",w);
				inbuf += get_line(inbuf,ww,0);
				outbuf = strapp(outbuf,"%");
				outbuf = strapp(outbuf,w);
				outbuf = strapp(outbuf,ww);
				if (*inbuf == NULL)	*outbuf++ = '\n';
				}
			}
		}

/* some manuals have commented lines beginning with ''' */
	else if ((c2 == '\'') && !(mid) && (c1 == '\n' || (first_word)))
		{
		if (*inbuf == '\'')
			{
			inbuf++;
			if (*inbuf == '\'')
				{
				inbuf++;
				outbuf = strapp(outbuf,"%");
				}
			else	outbuf = strapp(outbuf,"''");
			}
		else	outbuf = strapp(outbuf,"'");
		}

/* See if it is one of these symbols */

	else if (strcmp(w,"#") == 0 || strcmp(w,"&") == 0 ||
		 strcmp(w,"{") == 0 || strcmp(w,"}") == 0 ||
		 strcmp(w,"%") == 0 || strcmp(w,"_") == 0 ||
		 strcmp(w,"~") == 0 || strcmp(w,"^") == 0 )
		{
		outbuf = strapp(outbuf,"\\");
		outbuf = strapp(outbuf,w);
		if (strcmp(w,"~") == 0 || strcmp(w,"^") == 0)
			outbuf = strapp(outbuf,"{}");
		}

	else if (strcmp(w,">") == 0 || strcmp(w,"<") == 0 || strcmp(w,"|") == 0)
		{
		sprintf(tmp,"$%s$",w);
		outbuf = strapp(outbuf,tmp);
		}

/* check for backslash commands */

	else if (strcmp(w,"\\") == 0)
		{
		if (*inbuf == ' ' || *inbuf == '\t' || *inbuf == '\n')
			{
			outbuf = strapp(outbuf,"\\");
			*outbuf++ = *inbuf++;
			}
		else if (*inbuf == NULL)	;
		else if (*inbuf == '-')
			{
			inbuf++;
			outbuf = strapp(outbuf,"--");
			}
		else if (*inbuf == '~' || *inbuf == '^')
			{
			inbuf++;
			outbuf = strapp(outbuf,"\\/");
			}
		else if (*inbuf == '0')
			{
			inbuf++;
			outbuf = strapp(outbuf,"\\ ");
			}
		else if (*inbuf == 'e')
			{
			inbuf++;
			outbuf = strapp(outbuf,"\\bs ");
			}
		else if (*inbuf == '\\')
			{
			inbuf++;
			if (*inbuf == '$' && de_arg > 0)
				{
				inbuf++;
				de_arg++;
				*outbuf++ = '#';
				}
			else
				outbuf = strapp(outbuf,"\\bs ");
			}
		else if (*inbuf == '`' || *inbuf == '\'')
			;				/* do nothing */
		else if (*inbuf == '"')
			{
			inbuf++;
			inbuf += get_line(inbuf,ww,0);
			outbuf = strapp(outbuf,"%");
			outbuf = strapp(outbuf,ww);
			}
		else if (*inbuf == '|')
			{
			inbuf++;
			outbuf = strapp(outbuf,"\\,");
			}
		else if (*inbuf == '&')
			inbuf++;
		else if (*inbuf == '(')
			{
			c1 = *++inbuf;
			c2 = *++inbuf;
			inbuf++;
			if (c1 == 'e' && c2 == 'm')
				outbuf = strapp(outbuf,"---");
			else if (c1 == 'd' && c2 == 'e')
				outbuf = strapp(outbuf,"$^\\circ$");
			else fprintf(stderr,
				"I am not prepared to handle \\(%c%c\n",c1,c2);
			}
		else if (*inbuf == 's')
			inbuf +=3;
		else if (*inbuf == '*')
			{
			c1 = *++inbuf;
			inbuf++;
			if (c1 == ':')
				outbuf = strapp(outbuf,"\\\"");
			else if (c1 == 'C')
				outbuf = strapp(outbuf,"\\v");
			else if (c1 == ',')
				outbuf = strapp(outbuf,"\\c");
			else if (c1 != '(')
				{
				sprintf(tmp,"\\%c",c1);
				outbuf = strapp(outbuf,tmp);
				}
			else
				{
				fprintf(stderr,
				"I am not prepared to handle \\*( cases\n");
				inbuf += 2;
				}
			if (c1 != '(')
				{
				c1 = *inbuf++;
				sprintf(tmp,"{%c}",c1);
				outbuf = strapp(outbuf,tmp);
				}
			}
		else if (*inbuf == 'f')
			{
			c1 = *++inbuf;
			inbuf++;
			if (c1 == '1' || c1 == 'R')
				{
				lastfont = thisfont;
				thisfont = 1;
				if (*inbuf == ' ' || *inbuf == '\t' ||
				    *inbuf == '\n' || *inbuf == '\f')
					{*outbuf++ = ' ';	inbuf++;}
				outbuf = strapp(outbuf,"%\n\\rm ");
				}
			else if (c1 == '2' || c1 == 'I')
				{
				lastfont = thisfont;
				thisfont = 2;
				if (*inbuf == ' ' || *inbuf == '\t' ||
				    *inbuf == '\n' || *inbuf == '\f')
					{*outbuf++ = ' ';	inbuf++;}
				outbuf = strapp(outbuf,"%\n\\it ");
				}
			else if (c1 == '3' || c1 == 'B')
				{
				lastfont = thisfont;
				thisfont = 3;
				if (*inbuf == ' ' || *inbuf == '\t' ||
				    *inbuf == '\n' || *inbuf == '\f')
					{*outbuf++ = ' ';	inbuf++;}
				outbuf = strapp(outbuf,"%\n\\bf ");
				}
			else if (c1 == 'P')
				{
/* preserve white space - from Nelson Beebe  */
				if (*inbuf == ' ' || *inbuf == '\t' ||
				    *inbuf == '\n' || *inbuf == '\f')
					{*outbuf++ = ' ';	inbuf++;}
				switch(lastfont)
					{
					case 1:
						outbuf = strapp(outbuf,"\\rm%\n");
						thisfont = 1;
						break;
					case 2:
						outbuf = strapp(outbuf,"\\it%\n");
						thisfont = 2;
						break;
					case 3:
						outbuf = strapp(outbuf,"\\bf%\n");
						thisfont = 3;
						break;
					default:
						outbuf = strapp(outbuf,"\\rm%\n");
						thisfont = 1;
						break;
					}
				}
			else fprintf(stderr,
				"I do not understand \\f%c yet\n",c1);
			}
		else
			{
			fprintf(stderr,"I am not prepared to handle \\%c\n",*inbuf);
			inbuf++;
			}
		}

/* if non of the above checks, its a dull word; copy it */

	else
		outbuf = strapp(outbuf,w);
	*outbuf = NULL;	ww[0] = NULL;  tmp[0] = NULL;	tmp2[0] = NULL;
	if (!no_word)	first_word = 0;
	}
/* if file end, close opened environments and delimitters */
if (rec == 0)
	{
	if (IP_stat)	outbuf = strapp(outbuf,"\\end{itemize}\n");
	if (QP_stat)	outbuf = strapp(outbuf,"\\end{quotation}\n");
	if (TP_stat)	outbuf = strapp(outbuf,"\\end{TPlist}\n");
	}

*outbuf = NULL;
}

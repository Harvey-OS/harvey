#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <ctype.h>
#include "parse.h"
#include "xpal.h"

int eof = 0;
int scan_line = 1;
int scan_value;
FILE *scan_fp;
char peek = 0;
char scan_text[STRLEN];

extern char library[];

int
scan_eof(void)
{
	return(eof);
}

void
unscan(int c)
{
	Fundo(c, scan_fp);
}

int
scan(void)
{
	char buf[BUFSIZ];	
	int in;

	for(;;)
	{
		in = fgetc(scan_fp);
		if (in == '\n') scan_line++;
		if (in < 0) eof = 1;
		if (in == '#')
		{
			(void) fgets(buf, sizeof(buf), scan_fp);
			scan_line++;
		}
		else
		if (!isspace(in)) return(in);
	}
}

void
parse_error(char *msg)
{
	fprint(2, "parse error in %s at line %d: %s\n", library, scan_line, msg);
	exit(1);
}

void
scan_error(char *msg)
{
	fprint(2, "scanner error at line %d: %s\n", scan_line, msg);
	exit(1);
}

int
scan_getc(FILE *fp)
{
	int c;
	c = fgetc(fp);
	if (c < 0)
		scan_error("unexpected EOF");
	return(c);
}

int
NUMBER(int c)
{
	int base = 10;
	if (!isdigit(c)) return(0);
	if (c == '0')
	{
		c = fgetc(scan_fp);
		if (c == 'x') base = 16;
		else
		if (isdigit(c))
		{
			base = 8;
			unscan(c);
		}
		else
		{
			unscan(c);
			scan_value = 0;
			return(1);
		}
	} /* end if */
	scan_value = 0;
	while (isxdigit(c))
	{
		switch (base)
		{
		case 8:
			if (!((c >= '0') && (c <= '7')))
				scan_error("illegal character in octal constant");
			scan_value = scan_value << 3 + (c - '0');
			break;
		case 10:
			scan_value = scan_value * 10 + (c - '0');
			break;
		case 16:
			scan_value = scan_value << 4 +
				(isdigit(c) ? (c - '0') :
				 isupper(c) ? (c - 'A') :
				 islower(c) ? (c - 'a') : 0);
			break;
		} /* end switch */
		c = scan_getc(scan_fp);
	} /* end while */
	unscan(c);
	return(scan_value);
}

int
ID(int c)
{
	char *scan_ptr;
	if (!(isalpha(c) || isdigit(c) || (c == '_'))) return(0);
	scan_ptr = scan_text;
	while (isalpha(c) || isdigit(c) || (c == '_') || (c == '/'))
	{
		*scan_ptr++ = toupper(c);
		c = scan_getc(scan_fp);
	} /* end while */
	*scan_ptr = 0;
	unscan(c);
	return(1);
}

int
STRING(void)
{
	char *scan_ptr;
	int c;
	c = scan();
	if (c != '"') return(0);
	scan_ptr = scan_text;
	c = scan_getc(scan_fp);
	while (c != '"')
	{
		if (c == '\\') c = scan_getc(scan_fp);
		*scan_ptr++ = c;
		c = scan_getc(scan_fp);
	} /* end while */
	*scan_ptr = '\0';
	return(1);
}

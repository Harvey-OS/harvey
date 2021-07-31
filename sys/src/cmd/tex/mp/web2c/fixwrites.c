/*
 * This program converts the pascal-type write/writeln's into fprintf's
 * or putc's.
 *
 * Tim Morgan   October 10, 1987
 */

#include <stdio.h>
#include "site.h"

#ifdef	ANSI
#include <string.h>
#define	index	strchr
void exit(int);
void main(int argc, char * *argv);
char *insert_long(char *cp);
void join(char *cp);
void do_blanks(int indent);
int whole(char *buf);
char *skip_balanced(char *cp);
int bare(char *cp, char c);
#else
#ifdef	SYSV
#define	index	strchr
#endif
extern char *index(), *strcpy();
#endif /*ANSI*/

#define	TRUE	1
#define	FALSE	0

char buf[BUFSIZ], filename[100], *file, *argp, args[100], *as, *cmd;
int tex = FALSE;

extern char *index(), *strcpy();

char *insert_long(cp)
char *cp;
{
    char tbuf[BUFSIZ];
    register int i;

    for (i=0; &buf[i] < cp; ++i)
	tbuf[i] = buf[i];
    (void) strcpy(&tbuf[i], "(long)");
    (void) strcpy(&tbuf[i+6], cp);
    (void) strcpy(buf, tbuf);
    return(cp+6);
}

void join(cp)
char *cp;
{
    char temp[BUFSIZ], *tp;

    if (!gets(temp)) return;
    *cp++ = ' ';
    for (tp=temp; *tp == ' '; ++tp);
    (void) strcpy(cp, tp);
}

void do_blanks(indent)
int indent;
{
    register int i;

    for (i=0; i<indent/8; i++)
	(void) putchar('\t');
    indent %= 8;
    for (i=0; i<indent; i++)
	(void) putchar(' ');
}

/*
 * Return true if we have a whole write/writeln statement.  We determine
 * this by matching parens, ignoring those within strings.
 */
whole(cp)
register char *cp;
{
    register int depth=0;

    while (cp && *cp) {
	switch (*cp) {
	case '(':
		++depth;
		break;
	case ')':
		--depth;
		break;
	case '"':
		for (++cp; cp && *cp && *cp != '"'; ++cp)
			if (*cp == '\\') ++cp;
		break;
	case '\'':
		++cp;
		if (*cp == '\\') ++cp;
		++cp;
		break;
	}
	++cp;
    }
    return(depth <= 0);
}


/* Skips to the next , or ), skipping over balanced paren pairs */
char *skip_balanced(cp)
char *cp;
{
    register int depth = 0;

    while (depth > 0 || (*cp != ',' && *cp != ')')) {
	switch (*cp) {
	case '(':
	    ++depth;
	    break;
	case ')':
	    --depth;
	    break;
	}
	++cp;
    }
    return(cp);
}

/* Return true if c appears, except inside a quoted string */
#ifdef	ANSI
bare(char *cp, char c)
#else
bare(cp, c)
char *cp, c;
#endif
{
    for (; *cp && *cp != c; ++cp) {
	if (*cp == '"') {
	    ++cp;	/* Skip over initial quotation mark */
	    while (*cp && *cp != '"') { /* skip to closing double quote */
		if (*cp == '\\') ++cp;
		++cp;
	    }
	}
	else if (*cp == '\'') {
	    ++cp;	/* Skip to contained char */
	    if (*cp == '\'') ++cp;	/* if backslashed, it's double */
	    ++cp;	/* Skip to closing single-quote mark */
	}
    }
    return(*cp);
}

#ifdef	ANSI
void
#endif
main(argc, argv)
char *argv[];
{
    register char *cp;
    int blanks_done, indent, i;

    for (i=1; i<argc; i++) switch (argv[i][1]) {
    case 't':
	tex = TRUE;
	break;
    default:
	(void) fprintf(stderr, "Unknown flag %s\n", argv[i]);
	exit(1);
    }

    while (gets(buf)) {
	blanks_done = FALSE;
	for (cp=buf; *cp; ++cp);
	while (*--cp == ' ');
	while (*cp == '.') {
	    join(cp+1);
	    while (*cp) ++cp;
	    while (*--cp == ' ');
	}
	for (cp=buf, indent=0; *cp == ' ' || *cp == '\t'; ++cp) {
		if (*cp == ' ') indent++;
		else indent += 8;
	}
	if (!*cp) {	/* All blanks, possibly with "{" */
	    (void) puts(buf);
	    continue;
	}
	if (*cp == '{') {
	    do_blanks(indent);
	    (void) putchar('{');
	    ++cp;
	    while (*cp == ' ' || *cp == '\t') ++cp;
	    blanks_done = TRUE;
	    if (!*cp) {
		(void) putchar('\n');
		continue;
	    }
	}
	if (!blanks_done) do_blanks(indent);
	if (strncmp(cp, "read ( input", 12) == 0) {
	    char variable_name[20];
	    if (sscanf(cp, "read ( input , %s )", variable_name) != 1)
		(void) fprintf(stderr, "sscanf failed\n"), exit(1);
	    (void) printf("%s = getint();\n", variable_name);
	    continue;
	}
	if (strncmp(cp, "lab", 3) == 0 && index(cp, ':')) {
	    do {
		(void) putchar(*cp);
	    } while (*cp++ != ':');
	    while (*cp == ' ') ++cp;
	    (void) putchar(' ');
	}
	if (strncmp(cp, "else write", 10) == 0) {
	    (void) puts("else");
	    do_blanks(indent);
	    cp += 5;
	    while (*cp == ' ') ++cp;
	}
	if (bare(cp, '{')) {
	    while (*cp != '{') {
		(void) putchar(*cp);
		++cp;
	    }
	    ++cp;
	    (void) puts("{");
	    indent += 4;
	    do_blanks(indent);
	    while (*cp == ' ') ++cp;
	}
	if (strncmp(cp, "write (", 7) && strncmp(cp, "writeln (", 9)) {
	    /* if not a write/writeln, just copy it to stdout and continue */
	    (void) puts(cp);
	    continue;
	}
	cmd = cp;
	while (!whole(buf))	/* Make sure we have whole stmt */
	    (void) gets(&buf[strlen(buf)]);
	while (*cp != '(') ++cp;
	++cp;
	while (*(cp+1) == ' ') ++cp;
	if (*(cp+1) == '"' || *(cp+1) == '\'' ||
	  strncmp(cp+1, "buffer", 6) == 0 ||
	  strncmp(cp+1, "dig", 3) == 0 ||
	  strncmp(cp+1, "xchr", 4) == 0 ||
	  strncmp(cp+1, "k ,", 3) == 0 ||
	  strncmp(cp+1, "s ,", 3) == 0)
	    (void) strcpy(filename, "stdout");
	else {
	    file = filename;
	    while (*cp != ',' && *cp != ')')
		*file++ = *cp++;
	    *file = '\0';
	}
	if (*cp == ')') {
	    (void) printf("(void) putc('\\n', %s);\n", filename);
	    continue;
	}
	argp = ++cp;
	as = args;
	while (*cp==' ') ++cp;
	while (*cp != ')') {
	    if (*cp == '\'' || strncmp(cp, "xchr", 4) == 0
	      || strncmp(cp, "ASCII04", 7) == 0
	      || strncmp(cp, "ASCII1", 6) == 0
	      || strncmp(cp, "nameoffile", 10) == 0
	      || strncmp(cp, "months", 6) == 0) {
		*as++ = '%';
		*as++ = 'c';
		if (tex && strncmp(cp, "xchr", 4) == 0) {
		    *cp = 'X';
		    cp = index(cp, '[');
		    *cp = '(';
		    cp = index(cp, ']');
		    *cp++ = ')';
		}
		else if (*cp == '\'')
			cp += 2;
	    }
	    else if (*cp == '"') {
		*as++ = '%';
		*as++ = 's';
		while (*++cp != '"') /* skip to end of string */
		    if (*cp == '\\') ++cp;	/* allow \" in string */
	    }
	    else {
		*as++ = '%';
		*as++ = 'l';
		*as++ = 'd';
		cp = insert_long(cp);
		cp = skip_balanced(cp);/* It's a numeric expression */
	    }
	    while (*cp != ',' && *cp != ')') ++cp;
	    while (*cp == ',' || *cp == ' ') ++cp;
	}
	if (strncmp(cmd, "writeln", 7) == 0) {
	    *as++ = '\\';
	    *as++ = 'n';
	}
	*as = '\0';
	if (strcmp(args, "%c") == 0) {
	    for (as = argp; *as; ++as);
	    while (*--as != ')');
	    *as = '\0';
	    (void) printf("(void) putc(%s, %s);\n", argp, filename);
	} else if (strcmp(args, "%s") == 0) {
	    (void) printf("(void) Fputs(%s, %s\n", filename, argp);
	} else {
	    (void) printf("(void) fprintf(%s, \"%s\", %s\n",
			  filename, args, argp);
	}
    }
    exit(0);
}

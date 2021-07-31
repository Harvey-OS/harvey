/*
 * This program changes locals to be register declarations.  This
 * produces code which runs 10% faster on some systems
 * (e.g., Vax-11/750, Sequent Balance).  Don't try to use this
 * program with other than TeX and Metafont in C.
 *
 * Tim Morgan   February 25, 1988
 */

#include <stdio.h>
#include "site.h"

char line[10240];

#ifdef	REGFIX		/* REST OF FILE */

#ifdef	SYSV
#define	index	strchr
#endif

extern char *index();

#define	Puts(s)	fputs(s, stdout)

char *types[] = {"ASCIIcode ", "schar ", "eightbits ", "scaled ",
	"glueord ", "halfword ", "hyphpointer ", "internalfontnumber ",
	"nonnegativeinteger ", "poolpointer ", "quarterword ", "smallnumber ",
	"strnumber ", "triepointer ", "integer ", "short "};
#define NUMTYPES (sizeof(types)/sizeof(types[0]))
int lens[NUMTYPES];


char *matchestype()
{
    register int i;

    if (line[0] != ' ' || line[1] != ' ')
	return(0);
    for (i=0; i<NUMTYPES; i++) {
	if (strncmp(&line[2], types[i], lens[i]) == 0 && index(line, '[') == 0){
	    return(line+2+lens[i]);
	}
    }
    return(0);
}


main()
{
    register int i;
#ifdef	vax
    register char *cp;
#endif

    for (i=0; i<NUMTYPES; i++)
	lens[i] = strlen(types[i]);

    /* Copy the declarations */
    while (gets(line) && strncmp(&line[10], "coerce", 6) !=0)
	puts(line);
    puts(line);

    while (gets(line)) {
#ifdef	vax
	if (cp = matchestype() ) {
	    Puts("  register long ");
	    puts(cp);
#else
	if ( matchestype() ) {
	    Puts("  register");
	    puts(line+1);
#endif
	} else
	    puts(line);
    }
#ifdef	vax	/* This appears to be needed on 4.3BSD/Vax systems */
    (void) fclose(stdout);		/* XXX */
#endif
    exit(0);
}

#else	/* NOT REGFIX */

/* If we don't want to use register variables, we just copy stdin to stdout. */

main()
{
    while (gets(line))
	puts(line);
}

#endif	/* NOT REGFIX */

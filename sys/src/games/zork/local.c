/* local.c -- dungeon functions which need local definition */

#include "funcs.h"

#ifdef __AMOS__
#include <moncal.h>
#endif

/* This function should return TRUE_ if it's OK for people to play the
 * game, FALSE_ otherwise.  If you have a working <time.h> library,
 * you can define NONBUSINESS to disallow play Monday to Friday, 9-5
 * (this is only checked at the start of the game, though).  For more
 * complex control you will have to write your own version of this
 * function.
 */

#ifdef NONBUSINESS
#ifdef BSD4_2
#include <sys/timeb.h>
#else /* ! BSD4_2 */
#include <time.h>
#endif /* ! BSD4_2 */
#endif /* NONBUSINESS */

logical protected()
{
#ifndef NONBUSINESS

    return TRUE_;

#else /* NONBUSINESS */

    time_t t;
    struct tm *q;

    (void)time(&t);
    q = localtime(&t);

    /* Return TRUE_ if it's Sunday or Saturday or before 9 or after 5 */

    if (q->tm_wday == 0 || q->tm_wday == 6)
	return TRUE_;
    else if (q->tm_hour < 9 || q->tm_hour >= 17)
	return TRUE_;
    else
	return FALSE_;

#endif /* NONBUSINESS */

}

#ifdef ALLOW_GDT

/* This function should return TRUE_ if the user is allowed to invoke the
 * game debugging tool by typing "gdt".  This isn't very useful without
 * the source code, and it's mainly for people trying to debug the game.
 */

logical wizard()
{
    return FALSE_;
}

#endif

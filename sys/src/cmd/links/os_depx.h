/*
 * Plan 9 OS-dependent code (other than plan9.c).
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#undef MAXINT
#undef SA_RESTART

enum {
	MAXINT = ~0U >> 1,
	SA_RESTART = 0,
};

#define loop_select plan9_select

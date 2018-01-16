/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


/* extensible Strings */
typedef struct String {
	Lock Lock;
	char	*base;	/* base of String */
	char	*end;	/* end of allocated space+1 */
	char	*ptr;	/* ptr into String */
	short	ref;
	unsigned char	fixed;
} String;

#define s_clone(s) s_copy((s)->base)
#define s_to_c(s) ((s)->base)
#define s_len(s) ((s)->ptr-(s)->base)

extern String*	s_append(String*, char*);
extern String*	s_array(char*, int);
extern String*	s_copy(char*);
extern void	s_free(String*);
extern String*	s_incref(String*);	
extern String*	s_memappend(String*, char*, int);
extern String*	s_nappend(String*, char*, int);
extern String*	s_new(void);
extern String*	s_newalloc(int);
extern String*	s_parse(String*, String*);
extern String*	s_reset(String*);
extern String*	s_restart(String*);
extern void	s_terminate(String*);
extern void	s_tolower(String*);
extern void	s_putc(String*, int);
extern String*	s_unique(String*);
extern String*	s_grow(String*, int);

#ifdef BGETC
extern int	s_read(Biobuf*, String*, int);
extern char	*s_read_line(Biobuf*, String*);
extern char	*s_getline(Biobuf*, String*);
typedef struct Sinstack Sinstack;
extern char	*s_rdinstack(Sinstack*, String*);
extern Sinstack	*s_allocinstack(char*);
extern void	s_freeinstack(Sinstack*);
#endif /* BGETC */

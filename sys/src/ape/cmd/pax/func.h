/* $Source: /u/mark/src/pax/RCS/func.h,v $
 *
 * $Revision: 1.3 $
 *
 * func.h - function type and argument declarations
 *
 * DESCRIPTION
 *
 *	This file contains function delcarations in both ANSI style
 *	(function prototypes) and traditional style. 
 *
 * AUTHOR
 *
 *     Mark H. Colburn, NAPS International (mark@jhereg.mn.org)
 *
 * Sponsored by The USENIX Association for public distribution. 
 *
 * Copyright (c) 1989 Mark H. Colburn.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Mark H. Colburn and sponsored by The USENIX Association. 
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _PAX_FUNC_H
#define _PAX_FUNC_H

/* Function Prototypes */

#ifdef __STDC__

extern Link    	       *linkfrom(char *, Stat *);
extern Link    	       *linkto(char *, Stat *);
extern char    	       *mem_get(uint);
extern char    	       *mem_str(char *);
extern char    	       *strerror(void);
extern int      	ar_read(void);
extern int      	buf_read(char *, uint);
extern int      	buf_skip(OFFSET);
extern int      	create_archive(void);
extern int      	dirneed(char *);
extern int      	read_archive(void);
extern int      	inentry(char *, Stat *);
extern int      	lineget(FILE *, char *);
extern int      	name_match(char *);
extern int      	name_next(char *, Stat *);
extern int      	nameopt(char *);
extern int      	open_archive(int);
extern int      	open_tty(void);
extern int      	openin(char *, Stat *);
extern int      	openout(char *, Stat *, Link *, int);
extern int      	pass(char *);
extern int      	passitem(char *, Stat *, int, char *);
extern int      	read_header(char *, Stat *);
extern int      	wildmat(char *, char *);
extern void     	buf_allocate(OFFSET);
extern void     	close_archive(void);
extern void     	fatal(char *);
extern void     	name_gather(void);
extern void     	name_init(int, char **);
extern void     	names_notfound(void);
extern void     	next(int);
extern int      	nextask(char *, char *, int);
extern void     	outdata(int, char *, OFFSET);
extern void     	outwrite(char *, uint);
extern void     	passdata(char *, int, char *, int);
extern void     	print_entry(char *, Stat *);
extern void     	warn();
extern void		warnarch(char *, OFFSET);
extern void     	write_eot(void);
extern void		get_archive_type(void);
extern struct group    *getgrgid();
extern struct group    *getgrnam();
extern struct passwd   *getpwuid();
extern char    	       *getenv(char *);
extern SIG_T   	      (*signal())();
extern Link            *islink(char *, Stat *);
extern char            *finduname(int);
extern char            *findgname(int);
extern int		findgid(char *gname);
extern char    	       *malloc();

#else /* !__STDC__ */

extern Link    	       *linkfrom();
extern Link    	       *linkto();
extern char    	       *mem_get();
extern char    	       *mem_str();
extern char    	       *strerror();
extern int      	ar_read();
extern int      	buf_read();
extern int      	buf_skip();
extern int      	create_archive();
extern int      	dirneed();
extern int      	read_archive();
extern int      	inentry();
extern int      	lineget();
extern int      	name_match();
extern int      	name_next();
extern int      	nameopt();
extern int      	open_archive();
extern int      	open_tty();
extern int      	openin();
extern int      	openout();
extern int      	pass();
extern int      	passitem();
extern int     	 	read_header();
extern int      	wildmat();
extern void     	buf_allocate();
extern void     	close_archive();
extern void     	fatal();
extern void     	name_gather();
extern void     	name_init();
extern void     	names_notfound();
extern void     	next();
extern int      	nextask();
extern void     	outdata();
extern void     	outwrite();
extern void     	passdata();
extern void     	print_entry();
extern void     	warn();
extern void     	warnarch();
extern void     	write_eot();
extern void		get_archive_type();
extern char    	       *getenv();
extern char    	       *malloc();
extern char    	       *strcat();
extern char    	       *strcpy();
extern char    	       *strncpy();
extern SIG_T   	      (*signal())();
extern OFFSET   	lseek();
extern struct group    *getgrgid();
extern struct group    *getgrnam();
extern struct passwd   *getpwuid();
extern struct tm       *localtime();
extern time_t          	time();
extern uint            	sleep();
extern void            	_exit();
extern void            	exit();
extern void            	free();
extern Link            *islink();
extern char            *finduname();
extern char            *findgname();
extern int		findgid();

#endif /* __STDC__ */
#endif /* _PAX_FUNC_H */

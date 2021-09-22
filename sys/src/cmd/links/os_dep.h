/*
 * Plan 9 OS-dependent code (other than plan9.c).
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#ifndef _OS_DEP_H
#define _OS_DEP_H

#include <pwd.h>
#include <grp.h>

/* hardcoded limit of 10 OSes in default.c */
#define SYS_UNIX 1
#define UNIX

#define inline
static inline int dir_sep(char x) { return x == '/'; }
#define NEWLINE	"\n"

#define FS_UNIX_RIGHTS
#define FS_UNIX_HARDLINKS
#define FS_UNIX_SOFTLINKS
#define FS_UNIX_USERS

#define SYSTEM_ID SYS_UNIX
#define SYSTEM_NAME "Unix"
#define DEFAULT_SHELL "/bin/sh"
#define GETSHELL getenv("SHELL")
#define USE_AF_UNIX

#define ASSOC_BLOCK
#define ASSOC_CONS_XWIN

#define THREAD_SAFE_LOOKUP

#endif /* #ifndef_OS_DEP_H */

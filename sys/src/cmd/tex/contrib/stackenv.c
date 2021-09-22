/* stackenv.c: routines that help to mimic shell scripts.

Copyright (C) 1997 Fabrice POPINEAU.

Adapted to DJGPP by Eli Zaretskii.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include <assert.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#include <kpathsea/config.h>
#include <kpathsea/c-pathch.h>
#include <kpathsea/c-pathmx.h>
#include <kpathsea/c-vararg.h>
#include <kpathsea/c-stat.h>
#include <kpathsea/debug.h>
#include <kpathsea/variable.h>
#ifndef _WIN32
#include <sys/param.h>
#endif

#include "stackenv.h"

string mktex_version_string = "2.0";

extern char *output;
extern char tmpdir[];
extern char *progname;

/* The global stack. */
static mod_env stack_env[256];
static int index_env = 0;

void mt_exit(int);
extern void output_and_cleanup(int);

#ifndef _WIN32
#include <unistd.h>

void flushall (void)
{
  /* Simulate `flushall' by only flushing standard streams.
     This should be enough, since the non-standard are going
     to be closed in a moment, and will be flushed then.  */
  fflush (stdout);
  fsync (fileno (stdout));
  fflush (stderr);
  fsync (fileno (stderr));
}
#endif

#ifndef _WIN32
#ifdef __DJGPP__

/* DJGPP-specific emulation of functions which aren't in the library
   and aren't ANSI/POSIX.  */

#include <errno.h>
#include <libc/file.h>

static void fcloseall_func (FILE *f)
{
  fflush (f);
  if (fileno (f) > 2)
    fclose(f);
  else
    fsync (fileno (f));
}

void fcloseall (void)
{
  _fwalk (fcloseall_func);
}

#else  /* not __DJGPP__ */

/* FIXME: is this enough on Unix?  */
static void fcloseall (void)
{
  flushall ();
}

#endif /* not __DJGPP__ */
#endif /* not _WIN32 */

void __cdecl
oops(const char *message, ...)
{
        va_list args;

        va_start(args, message);
        vfprintf(stderr, message, args);
        va_end(args);
        fputc('\n', stderr);
        mt_exit(1);
}

/* pushd */
void pushd(char *p)
{
  if (index_env >= sizeof(stack_env)/sizeof(stack_env[0])) {
    fprintf(stderr, "%s: stack overflow in pushd\n", progname);
    mt_exit(1);
  }
  if ((stack_env[index_env].data.path = malloc(MAXPATHLEN)) == NULL
      || getcwd(stack_env[index_env].data.path, MAXPATHLEN) == NULL) {
    fprintf(stderr, "pushd error!\n");
    mt_exit(1);
  }
  stack_env[index_env].op = CHDIR;
  if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
    fprintf(stderr, "At %d, pushing from %s to %s\n", index_env, 
	    stack_env[index_env].data.path, p);
  }

  index_env++;
  if (chdir(p) == -1) {
    perror(p);
    mt_exit(1);
  }
}
/* popd */
void popd()
{
  if (index_env == 0)
    return;
  index_env--;
  if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
    fprintf(stderr, "At %d, popping %s\n", index_env, stack_env[index_env].data.path);
  }
  assert(stack_env[index_env].op == CHDIR);
  if (chdir(stack_env[index_env].data.path) == -1) {
    perror(stack_env[index_env].data.path);
    mt_exit(1);
  }
  free(stack_env[index_env].data.path);
  return;
}

/* random access to directories' stack  */
char *peek_dir(int n)
{
  int i;
  int level = n;

  /* The stack holds both directories pushed by `pushd' and file
     handles pushed by `push_fd'.  When they say "peek_dir(n)", they
     mean the n'th DIRECTORY entry, not the n'th entry counting from
     the stack bottom.

     For example, if the stack looks like this:

		  idx      type

		   0	  handles
		   1	  handles
		   2	  directory
		   3	  handles    <--- top of the stack
		   4      <empty>    <--- index_env

     then "peek_dir(0)" should return the entry with the index 2, not
     the one with the index 0.

     The following loop looks for the index i of a stack slot which
     holds the n'th directory pushed on to the stack.  */

  do {
    for (i = 0; i < index_env && stack_env[i].op != CHDIR; i++)
      ;
  } while (i < index_env && n--);

  if (i < index_env)
    return stack_env[i].data.path;
  else {
    fprintf (stderr, "%s: attempt to peek at invalid stack level %d\n",
	     progname, level);
    mt_exit(1);
  }
}

/* redirect */
void push_fd(int newfd[3])
{
  int i;

  if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
    fprintf(stderr, "At %d, pushing fds %d %d %d\n", index_env, newfd[0], 
	    newfd[1], newfd[2]);
  }
  flushall();
  if (index_env >= sizeof(stack_env)/sizeof(stack_env[0])) {
    fprintf(stderr, "%s: stack overflow in push_fd\n", progname);
    mt_exit(1);
  }
  stack_env[index_env].op = REDIRECT;
  for(i = 0; i < 3; i++) {
    if (i == newfd[i])
      stack_env[index_env].data.oldfd[i] = i;
    else {
      if ((stack_env[index_env].data.oldfd[i] = dup(i)) == -1) {
	perror("push_fd: dup");
	mt_exit(1);
      }
      if (dup2(newfd[i], i) == -1) {
	perror("push_fd : dup2");
	mt_exit(1);
      }
    }
  }
  index_env++;
}

void pop_fd()
{
  int i;
  index_env--;
  assert(stack_env[index_env].op == REDIRECT);
  if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
    fprintf(stderr, "At %d; popping fds %d %d %d\n", 
	    index_env,
	    stack_env[index_env].data.oldfd[0],
	    stack_env[index_env].data.oldfd[1],
	    stack_env[index_env].data.oldfd[2]);
  } 
  flushall();
  for(i = 0; i < 3; i++)
    if (i != stack_env[index_env].data.oldfd[i]) {
      close(i);
      if (dup2(stack_env[index_env].data.oldfd[i], i) == -1) {
	perror("pop_fd : dup2");
	mt_exit(1);
      }
      close(stack_env[index_env].data.oldfd[i]);
    }
}

/* popenv */
void popenv()
{
  switch(stack_env[index_env-1].op) {
  case CHDIR:
    popd();
    break;
  case REDIRECT:
    pop_fd();
    break;
  default:
    fprintf(stderr, "popenv : unknown op %d.\n", stack_env[index_env-1].op);
    break;
  }
}

#ifdef _WIN32
BOOL sigint_handler(DWORD dwCtrlType)
{
  /* Fix me : there is a problem if a system() command is running.
     We should wait for the son process to be interrupted.
     Only way I can think of to do that : rewrite system() based on
     spawn() with parsing of the command line and set a global pid
     Next cwait(pid) in the HandlerRoutine. 
     */
  mt_exit(3);
  return FALSE;			/* return value obligatory */
}
#else
void sigint_handler (int sig)
{
  mt_exit(3);
}
#endif

void mt_exit(int code)
{
  /* rétablir les 0, 1 et 2 d'origine avant de tenter quoi que ce soit ! */
  fcloseall();

  /* unstack all env from main) */
  for( ; index_env > 0; popenv());
  /* output the result and rm any unneeded file */
  output_and_cleanup(code);

  exit(code);
}

/* A few utility functions shared by both mktex and makempx.  */

/* Copied from kpathsea/progname.c */
string
dirname P1C(const_string, name)
{
  string ret;
  unsigned loc = strlen (name);
  
  for ( ; loc > 0 && !IS_DIR_SEP (name[loc-1]); loc--)
    ;

  if (loc) {
    /* If have ///a, must return /, so don't strip off everything.  */
    unsigned limit = NAME_BEGINS_WITH_DEVICE (name) ? 3 : 1;
    while (loc > limit && IS_DIR_SEP (name[loc-1])
           && !IS_DEVICE_SEP (name[loc-1])) {
      loc--;
    } 
    ret = xmalloc (loc + 1);
    strncpy (ret, name, loc);
    ret[loc] = 0;
  } else {
    /* No directory separators at all, so return the current directory.
       The way this is used in kpathsea, this should never happen.  */
    ret = xstrdup (".");
  }
  
  return ret;
}

void
dump_stack()
{
  int i;
  for (i = 0; i < index_env; i++)
    switch (stack_env[i].op) {
    case CHDIR:
      fprintf(stderr, "Env %d : chdir %s\n", i, stack_env[i].data.path);
      break;
    case REDIRECT:
      fprintf(stderr, "Env %d : redirect %d %d %d\n", i, 
	      stack_env[i].data.oldfd[0],
	      stack_env[i].data.oldfd[1],
	      stack_env[i].data.oldfd[2]);
      break;
    }

}

/* Execute a command, much like system() but using popen() */
int 
execute_command(string cmd)
{
  int retcode = 0;
  FILE *fcmd = NULL;
  if ((fcmd = popen(cmd, "r")) == NULL)
    return -1;
  retcode = pclose(fcmd);
  if (KPSE_DEBUG_P(MKTEX_FINE_DEBUG)) {
    fprintf(stderr, "executing %s => %d\n", cmd, retcode);
  }
  return retcode;
}

/* Copy a file, appending if required.  */

int catfile (const char *from, const char *to, int append)
{
  FILE *finp, *fout;
  unsigned char buf[16*1024];
  int rbytes;

  finp = fopen (from, "rb");
  if (!finp)
    return FALSE;
  fout = fopen (to, append ? "ab" : "wb");
  if (!fout) {
    fclose (finp);
    return FALSE;
  }

  while ((rbytes = fread (buf, 1, sizeof buf, finp)) > 0) {
    int wbytes = fwrite (buf, 1, rbytes, fout);

    if (wbytes < rbytes) {
      if (wbytes >= 0)
	fprintf (stderr, "%s: disk full\n", to);
      fclose (finp);
      fclose (fout);
      unlink (to);
      return FALSE;
    }
  }

  fclose (finp);
  fclose (fout);
  return TRUE;
}

/* Move a file using rename if possible, copy if necessary.  */

int mvfile (const char *from, const char *to)
{
  int e = errno;
  int retval = TRUE;
  char errstring[PATH_MAX*4];

  errno = 0;
  if (rename (from, to) == 0) {
    errno = e;
    return TRUE;
  }

#ifndef DOSISH
  /* I don't want to rely on this on DOSISH systems.  First, not all
     of them know about EXDEV.  And second, DOS will fail `rename'
     with EACCES when the target is more than 8 directory levels deep.  */
  if (errno != EXDEV)
    retval = FALSE;
  else
#endif

  /* `rename' failed on cross-filesystem move.  Copy, then delete.  */
  if (catfile (from, to, 0) == FALSE)
    retval = FALSE;
  else
    unlink (from);	/* ignore possible errors */

  /* Simulate error message from `mv'.  */
  if (retval == FALSE) {
    sprintf(errstring, "%s: can't move %s to %s", progname, from, to);
    perror(errstring);
  }

  if (!errno)
    errno = e;
  return retval;
}

/* Like the test function from Unix */

boolean test_file(int c, string path)
{
  struct stat stat_buf;
  boolean file_exists;

  if (c == 'z')
    return (path == NULL || *path == '\0');

  if (c == 'n')
    return (path != NULL && *path != '\0');
  
#ifdef WIN32
  /* Bug under W95 */
  if (c == 'd')
    return dir_p(path);
#endif

  file_exists = (stat(path, &stat_buf) != -1);

#ifdef DOSISH
  /* When they say "test -x foo", make so we find "foo.exe" etc.  */
  if (c == 'x' && !file_exists)
    {
      static string try_ext[] = {
	".exe",
	".bat",
	".cmd",
	".btm",
	0
      };
      string *p = try_ext;

      while (p)
	{
	  string try = concat(path, *p++);
	  if (stat(try, &stat_buf) != -1)
	    {
	      file_exists = 1;
	      free(try);
	      break;
	    }
	  free(try);
	}
    }
#endif

  if (!file_exists) return FALSE;
  switch (c) {
  case 'd':
    return ((stat_buf.st_mode & S_IFMT) == S_IFDIR);
  case 'e':
  case 'r':
    return TRUE;
  case 'f':
    return ((stat_buf.st_mode & S_IFMT) == S_IFREG);
  case 's':
    return (stat_buf.st_size > 0);
  case 'x':
    return ((stat_buf.st_mode & S_IFMT) == S_IFREG
	    && (stat_buf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)));
  case 'w':
    {
      return (
#ifdef DOSISH
	      /* DOSISH systems allow to create files in read-only
		 directories, and `is_writable' knows about that.  So we
		 artificially look at the write bit returned by `stat'
		 and refuse to write in  read-only directories.  */
	      (stat_buf.st_mode & S_IWUSR) != 0 &&
#endif
	      is_writable(path));
    }
  default:
    return FALSE;
  }
}

/*
  This is used by mktex{mf,tfm,pk}
  */
void start_redirection(void)
{
  int newfd[3];
  FILE *fnul;
#ifdef _WIN32
  fnul = fopen("nul", "r");	/* fopen /dev/null */
#else
  fnul = fopen("/dev/null", "r");
#endif
  
  newfd[0] = fileno(fnul);
  newfd[1] = 2;
  newfd[2] = 2;
  
  push_fd(newfd);
}

#ifdef _WIN32

boolean is_writable(string path)
{
  DWORD dw;
  char tmppath[256];
  int writable;

  /* Test 2 cases: directory or file */
  if ((dw = GetFileAttributes(path)) == 0xFFFFFFFF) {
    /* Invalid Path, not writable */
    writable = false;
    errno = ENOENT;	/* like `access' does */
  }
  else if (dw & FILE_ATTRIBUTE_READONLY) {
    writable = false;
  }
  else if (dw & FILE_ATTRIBUTE_DIRECTORY) {
    /* Trying to open a file */
    if (GetTempFileName(path, "foo", 0, tmppath) == 0) {
      writable = false;
    }
    else {
      writable = true;
    }
    DeleteFile(tmppath);
  }
  else {
    writable = true;
  }
  return writable;
}

#else  /* !_WIN32 */

boolean is_writable(string path)
{
  if (!path || !*path)	/* like `access' called with empty variable as arg */
    return false;
#ifdef MSDOS
  /* Directories always return as writable, even on read-only filesystems
     such as CD-ROMs or write-protected floppies.  Need to try harder...  */
  if (access(path, D_OK) == 0) {
    /* Use a unique file name.  */
    string template = concat3(path, "/", "wrXXXXXX");
    int fd;

    if (mktemp(template) && (fd = creat(template, S_IRUSR | S_IWUSR)) >= 0) {
      close(fd);
      unlink(template);
      free(template);
      return true;
    }

    free(template);

    /* Simulate errno from `access'.  */
#ifdef EROFS
    errno = EROFS;	/* POSIX platforms should have this */
#else
    errno = EACCES;
#endif
    return false;
  }
  else
#endif /* MSDOS */
  return access(path, W_OK) == 0 ? true : false;
}

#endif /* !_WIN32 */

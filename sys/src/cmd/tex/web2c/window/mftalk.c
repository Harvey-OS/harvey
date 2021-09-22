/* mftalk.c -- generic Metafont window server.
   Copyright (C) 1994 Ralph Schleicher
   Slightly modified for Web2c 7.0 by kb@mail.tug.org.
   Further modifications for Web2C 7.2 by Mathias.Herberts@irisa.fr  */

/* This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Please remember the following for porting to UNIX:

	pid = fork ();
   	if (pid == 0)
   	  execve (...);		pid = spawnve (mode, ...);
   	else if (pid == -1)	if (pid == -1)
   	  error ();		  error ();
   	else			else
   	  success ();		  success ();

   Spawnve(2) has many different modes and a `session' is indicated by
   running on an extra terminal.  */


#define	EXTERN		extern
#include "../mfd.h"

#ifdef MFTALKWIN

#undef input
#undef output
#undef read
#undef write
#ifdef OS2
#include <sys/param.h>
#include <process.h>
extern int close (int);
extern int pipe (int *);
extern int read (int, void *, size_t);
extern int setmode (int, int);
extern int write (int, const void *, size_t);
#endif
#include <fcntl.h>
#include <signal.h>
#include "mftalk.h"

#define fatal(func, cond) do { if (cond) FATAL_PERROR ("perror"); } while (0)

static RETSIGTYPE child_died P1H(int sig);
static string app_type P2H(char *prog, int *app);

static int pid = -1;			/* Process ID of our child. */
static int win = -1;			/* Write handle to the `window'. */
static int buf[8];			/* Temporary buffer. */
static RETSIGTYPE (*old) ();		/* Old signal handler. */


boolean
mf_mftalk_initscreen P1H(void)
{
  int app;				/* Client application type. */
  char *prog, *name;			/* Client program name. */
    /* Size of METAFONT window. */
  char height[MAX_INT_LENGTH], width[MAX_INT_LENGTH];
    /* Inherited pipe handles. */
  char input[MAX_INT_LENGTH], output[MAX_INT_LENGTH];
  char parent[MAX_INT_LENGTH];		/* My own process ID. */
  int sc_pipe[2];			/* Server->Client pipe. */
  int cs_pipe[2];			/* Client->Server pipe. */
  int res, ack;				/* Wait until child is ready. */

  prog = kpse_var_value ("MFTALK");
  if (prog == NULL)
    prog = "mftalk.exe";

  name = app_type (prog, &app);
  if (!name)
    return 0;

  if (pipe (sc_pipe) == -1)
    return 0;
  if (pipe (cs_pipe) == -1)
    {
      close (sc_pipe[0]);
      close (sc_pipe[1]);
      return 0;
    }
#ifdef OS2
  fatal (setmode, setmode (sc_pipe[0], O_BINARY) == -1);
  fatal (setmode, setmode (sc_pipe[1], O_BINARY) == -1);
  fatal (setmode, setmode (cs_pipe[0], O_BINARY) == -1);
  fatal (setmode, setmode (cs_pipe[1], O_BINARY) == -1);
#endif

  old = signal (SIGCLD, child_died);
  fatal (old, old == SIG_ERR);

  sprintf (height, "-h%d", screendepth);
  sprintf (width, "-w%d", screenwidth);
  sprintf (input, "-i%d", sc_pipe[0]);
  sprintf (output, "-o%d", cs_pipe[1]);
  sprintf (parent, "-p%d", getpid ());

#ifdef OS2
  pid = spawnl (app, name, prog, height, width, input, output, parent, NULL);
#else
  pid = fork ();
  if (pid == 0)
    {
      fatal (close, close (0) == -1);
      fatal (dup, dup (sc_pipe[0]) != 0);
      fatal (close, close (sc_pipe[0]) == -1);
      fatal (close, close (sc_pipe[1]) == -1);      
      fatal (close, close (1) == -1);
      fatal (dup, dup (cs_pipe[1]) != 1);
      fatal (close, close (cs_pipe[0]) == -1);
      fatal (close, close (cs_pipe[1]) == -1);      
      
      /* We still pass the file handles as parameters for
       * backward compatibility. instead of sc_pipe[0] and
       * cs_pipe[1] we just pass 0 (stdin) and 1 (stdout).
       */

      sprintf (input, "-i0");
      sprintf (output, "-o1");
      
      execl (name, prog, height, width, input, output, parent, NULL);
    }
#endif /* not OS2 */
  switch (pid)
    {
    case -1:
    failure:
      fatal (close, close (sc_pipe[0]) == -1);
      fatal (close, close (sc_pipe[1]) == -1);
      fatal (close, close (cs_pipe[0]) == -1);
      fatal (close, close (cs_pipe[1]) == -1);
      fatal (signal, signal (SIGCLD, old) == SIG_ERR);
      break;
    default:
      res = read (cs_pipe[0], &ack, sizeof (int));
      if (res != sizeof (int) || ack != MF_ACK)
	goto failure;
      fatal (close, close (cs_pipe[0]) == -1);
      win = sc_pipe[1];
      break;
    }

  return (win == -1) ? 0 : 1;
}


void
mf_mftalk_updatescreen P1H(void)
{
  buf[0] = MF_FLUSH;
  write (win, buf, sizeof (int));
}


void
mf_mftalk_blankrectangle P4C(screencol, left,
                             screencol, right,
                             screenrow, top,
                             screenrow, bottom)
{
  buf[0] = MF_RECT;
  buf[1] = MF_WHITE;
  buf[2] = left;
  buf[3] = bottom;
  buf[4] = right;
  buf[5] = top;

  write (win, buf, 6 * sizeof (int));
}


void
mf_mftalk_paintrow P4C(screenrow, row,
                       pixelcolor, init_color,
                       transspec, transition_vector,
                       screencol, vector_size)
{
  buf[0] = MF_LINE;
  buf[1] = init_color == 0 ? MF_WHITE : MF_BLACK;
  buf[2] = *transition_vector++;
  buf[3] = row;
  buf[4] = --vector_size;

  write (win, buf, 5 * sizeof (int));
  write (win, transition_vector, vector_size * sizeof (int));
}


static string
app_type P2C(string, prog,  int *, app)
{
#ifdef OS2
  int res, app;

  res = DosSearchPath (0x02 | 0x01, "PATH", prog, buf, len);
  if (res != 0)
    return -1;

  res = DosQueryAppType (buf, &app);
  if (res != 0)
    return -1;

  switch (app & 0x07)			/* Quick guess. */
    {
    case 0x00: return (P_SESSION | P_DEFAULT);
    case 0x01: return (P_SESSION | P_FULLSCREEN);
    case 0x02: return (P_SESSION | P_WINDOWED);
    case 0x03: return (P_PM);
    }
#endif /* OS2 */

  *app = 0; /* Irrelevant.  */
  return prog;
}


static RETSIGTYPE
child_died (int sig)
{
#ifdef OS2
  fatal (signal, signal (sig, SIG_ACK) == SIG_ERR);
#endif
  fatal (signal, signal (sig, SIG_IGN) == SIG_ERR);

  if (pid == -1 || kill (-pid, 0) == 0)	/* This was not our child. */
    {
      if (old != SIG_IGN)
	{
	  fatal (signal, signal (sig, old) == SIG_ERR);
	  fatal (raise, raise (sig) == -1);
	}
      fatal (signal, signal (sig, child_died) == SIG_ERR);
    }
  else
    {
      close (win);			/* This may fail. */
      win = -1;

      pid = -1;

      screenstarted = false;		/* METAFONT variables. */
      screenOK = false;

      fatal (signal, signal (sig, old) == SIG_ERR);
    }
}

#else /* !MFTALKWIN */

int mftalk_dummy;

#endif /* !MFTALKWIN */

/* texmf.c: Hand-coded routines for TeX or Metafont in C.  Originally
   written by Tim Morgan, drawing from other Unix ports of TeX.  This is
   a collection of miscellany, everything that's easier (or only
   possible) to do in C.
   
   This file is public domain.  */

#define	EXTERN /* Instantiate data from {tex,mf,mp}d.h here.  */

/* This file is used to create texextra.c etc., with this line
   changed to include texd.h, mfd.h, or mpd.h.  The ?d.h file is what
   #defines TeX or MF or MP, which avoids the need for a special
   Makefile rule.  */
#include "mpd.h"

#include <kpathsea/config.h>
#include <kpathsea/c-ctype.h>
#include <kpathsea/line.h>
#include <kpathsea/readable.h>
#include <kpathsea/variable.h>
#include <kpathsea/absolute.h>

#include <time.h> /* For `struct tm'.  */
#ifndef WIN32
extern struct tm *localtime ();
#endif

#if defined(__STDC__)
#include <locale.h>
#endif

#include <signal.h> /* Catch interrupts.  */

/* {tex,mf}d.h defines TeX, MF, INI, and other such symbols.
   Unfortunately there's no way to get the banner into this code, so
   just repeat the text.  */
#ifdef TeX
#if defined (eTeX)
#include <etexdir/etexextra.h>
#elif defined (PDFTeX)
#include <pdftexdir/pdftexextra.h>
#elif defined (Omega)
#include <omegadir/omegaextra.h>
#else
#define BANNER "This is TeX, Version 3.14159"
#define COPYRIGHT_HOLDER "D.E. Knuth"
#define AUTHOR NULL
#define PROGRAM_HELP TEXHELP
#define DUMP_VAR TEXformatdefault
#define DUMP_LENGTH_VAR formatdefaultlength
#define DUMP_OPTION "fmt"
#define DUMP_EXT ".fmt"
#define INPUT_FORMAT kpse_tex_format
#define INI_PROGRAM "initex"
#define VIR_PROGRAM "virtex"
#endif
#define edit_var "TEXEDIT"
#endif /* TeX */
#ifdef MF
#define BANNER "This is Metafont, Version 2.718"
#define COPYRIGHT_HOLDER "D.E. Knuth"
#define AUTHOR NULL
#define PROGRAM_HELP MFHELP
#define DUMP_VAR MFbasedefault
#define DUMP_LENGTH_VAR basedefaultlength
#define DUMP_OPTION "base"
#ifdef DOS
#define DUMP_EXT ".bas"
#else
#define DUMP_EXT ".base"
#endif
#define INPUT_FORMAT kpse_mf_format
#define INI_PROGRAM "inimf"
#define VIR_PROGRAM "virmf"
#define edit_var "MFEDIT"
#endif /* MF */
#ifdef MP
#define BANNER "This is MetaPost, Version 0.64"
#define COPYRIGHT_HOLDER "AT&T Bell Laboratories"
#define AUTHOR "John Hobby"
#define PROGRAM_HELP MPHELP
#define DUMP_VAR MPmemdefault
#define DUMP_LENGTH_VAR memdefaultlength
#define DUMP_OPTION "mem"
#define DUMP_EXT ".mem"
#define INPUT_FORMAT kpse_mp_format
#define INI_PROGRAM "inimpost"
#define VIR_PROGRAM "virmpost"
#define edit_var "MPEDIT"
#endif /* MP */

/* The main program, etc.  */

/* What we were invoked as and with.  */
char **argv;
int argc;

/* If the user overrides argv[0] with -progname.  */
static string user_progname;

/* The C version of what might wind up in DUMP_VAR.  */
static string dump_name;

/* The main body of the WEB is transformed into this procedure.  */
extern void mainbody P1H(void);

static void maybe_set_dump_default_from_input P1H(string *);
static void parse_options P2H(int, string *);

#if defined(WIN32)
  /* if _DEBUG is not defined, these macros will result in nothing. */
   SETUP_CRTDBG;
   /* Set the debug-heap flag so that freed blocks are kept on the
    linked list, to catch any inadvertent use of freed memory */
   SET_CRT_DEBUG_FIELD( _CRTDBG_DELAY_FREE_MEM_DF );
#endif

/* The entry point: set up for reading the command line, which will
   happen in `topenin', then call the main body.  */

int
main P2C(int, ac,  string *, av)
{
#ifdef __EMX__
  _wildcard (&ac, &av);
  _response (&ac, &av);
#endif

  /* Save to pass along to topenin.  */
  argc = ac;
  argv = av;

  /* Must be initialized before options are parsed.  */
  interactionoption = 4;

#if defined(__STDC__)
  /* "" means: get value from env. var LC_ALL, LC_CTYPE, or LANG */
  setlocale(LC_CTYPE, "");
#endif

  /* If the user says --help or --version, we need to notice early.  And
     since we want the --ini option, have to do it before getting into
     the web (which would read the base file, etc.).  */
  parse_options (ac, av);
  
  /* Do this early so we can inspect program_invocation_name and
     kpse_program_name below, and because we have to do this before
     any path searching.  */
  kpse_set_program_name (argv[0], user_progname);

  /* If no dump default yet, and we're not doing anything special on
     this run, look at the first line of the main input file for a
     %&<dumpname> specifier.  */
  if (!dump_name) {
    maybe_set_dump_default_from_input (&dump_name);
  }
  
  /* If we're preloaded, I guess everything is set up.  I don't really
     know any more, it's been so long since anyone preloaded.  */
  if (readyalready != 314159) {
    /* The `ini_version' variable is declared/used in the change files.  */
    boolean virversion = false;
    if (FILESTRCASEEQ (kpse_program_name, INI_PROGRAM)) {
      iniversion = true;
    } else if (FILESTRCASEEQ (kpse_program_name, VIR_PROGRAM)) {
      virversion = true;
#ifdef TeX
#ifndef Omega
    } else if (FILESTRCASEEQ (kpse_program_name, "mltex")) {
      mltexp = true;
#endif /* !Omega */
#ifdef eTeX  /* For e-TeX compatibility mode... */
    } else if (FILESTRCASEEQ (kpse_program_name, "initex")) {
      iniversion = true;
    } else if (FILESTRCASEEQ (kpse_program_name, "virtex")) {
      virversion = true;
#endif /* eTeX */
#endif /* TeX */
    }

    if (!dump_name) {
      /* If called as *vir{mf,tex,mpost} use `plain'.  Otherwise, use the
         name we were invoked under.  */
      dump_name = (virversion ? "plain" : kpse_program_name);
    }
  }

  /* If we've set up the fmt/base default in any of the various ways
     above, also set its length.  */
  if (dump_name) {
    /* adjust array for Pascal and provide extension */
    DUMP_VAR = concat3 (" ", dump_name, DUMP_EXT);
    DUMP_LENGTH_VAR = strlen (DUMP_VAR + 1);
  } else {
    /* For dump_name to be NULL is a bug.  */
    abort();
  }

  /* Additional initializations.  No particular reason for doing them
     here instead of first thing in the change file; less symbols to
     propagate through Webc, that's all.  */
#ifdef MF
  kpse_set_program_enabled (kpse_mf_format, MAKE_TEX_MF_BY_DEFAULT,
                            kpse_src_compile);
#endif /* MF */
#ifdef TeX
#ifdef Omega
  kpse_set_program_enabled (kpse_ocp_format, MAKE_OMEGA_OCP_BY_DEFAULT,
                            kpse_src_compile);
  kpse_set_program_enabled (kpse_ofm_format, MAKE_OMEGA_OFM_BY_DEFAULT,
                            kpse_src_compile);
  kpse_set_program_enabled (kpse_tfm_format, false, kpse_src_compile);
  kpse_set_program_enabled (kpse_tex_format, MAKE_TEX_TEX_BY_DEFAULT,
                            kpse_src_compile);
#else
  kpse_set_program_enabled (kpse_tfm_format, MAKE_TEX_TFM_BY_DEFAULT,
                            kpse_src_compile);
  kpse_set_program_enabled (kpse_tex_format, MAKE_TEX_TEX_BY_DEFAULT,
                            kpse_src_compile);
#endif /* !Omega */
  if (!shellenabledp) {
    string shell_escape = kpse_var_value ("shell_escape");
    shellenabledp = (shell_escape
                     && (*shell_escape == 't'
                         || *shell_escape == 'y'
                         || *shell_escape == '1'));
  }
  if (!outputcomment) {
    outputcomment = kpse_var_value ("output_comment");
  }
#endif /* TeX */

  /* Call the real main program.  */
  mainbody ();
  return EXIT_SUCCESS;
} 


/* This is supposed to ``open the terminal for input'', but what we
   really do is copy command line arguments into TeX's or Metafont's
   buffer, so they can handle them.  If nothing is available, or we've
   been called already (and hence, argc==0), we return with
   `last=first'.  */

void
topenin P1H(void)
{
  int i;

  buffer[first] = 0; /* In case there are no arguments.  */

  if (optind < argc) { /* We have command line arguments.  */
    int k = first;
    for (i = optind; i < argc; i++) {
      char *ptr = &(argv[i][0]);
      /* Don't use strcat, since in Omega the buffer elements aren't
         single bytes.  */
      while (*ptr) {
        buffer[k++] = *(ptr++);
      }
      buffer[k++] = ' ';
    }
    argc = 0;	/* Don't do this again.  */
    buffer[k] = 0;
  }

  /* Find the end of the buffer.  */
  for (last = first; buffer[last]; ++last)
    ;

  /* Make `last' be one past the last non-blank character in `buffer'.  */
  for (--last; last >= first
       && isblank (buffer[last]) && buffer[last] != '\r'; --last) 
    ;
  last++;

  /* One more time, this time converting to TeX's internal character
     representation.  */
#ifndef Omega
#ifdef NONASCII
  for (i = first; i < last; i++)
    buffer[i] = xord[buffer[i]];
#endif
#endif
}

/* IPC for TeX.  By Tom Rokicki for the NeXT; it makes TeX ship out the
   DVI file in a pipe to TeXView so that the output can be displayed
   incrementally.  Shamim Mohamed adapted it for Web2c.  */
#if defined (TeX) && defined (IPC)

#include <sys/socket.h>
#include <fcntl.h>
#ifndef O_NONBLOCK /* POSIX */
#ifdef O_NDELAY    /* BSD */
#define O_NONBLOCK O_NDELAY
#else
#ifdef FNDELAY     /* NeXT */
#define O_NONBLOCK O_FNDELAY
#else
what the fcntl? cannot implement IPC without equivalent for O_NONBLOCK.
#endif /* no FNDELAY */
#endif /* no O_NDELAY */
#endif /* no O_NONBLOCK */

#ifndef IPC_PIPE_NAME /* $HOME is prepended to this.  */
#define IPC_PIPE_NAME "/.TeXview_Pipe"
#endif
#ifndef IPC_SERVER_CMD /* Command to run to start the server.  */
#define IPC_SERVER_CMD "open `which TeXview`"
#endif

struct msg
{
  short namelength; /* length of auxiliary data */
  int eof;   /* new eof for dvi file */
#if 0  /* see usage of struct msg below */
  char more_data[0]; /* where the rest of the stuff goes */ 
#endif
};

static char *ipc_name;
static struct sockaddr *ipc_addr;
static int ipc_addr_len;

static int
ipc_make_name P1H(void)
{
  if (ipc_addr_len == 0) {
    string s = getenv ("HOME");
    if (s) {
      ipc_addr = xmalloc (strlen (s) + 40);
      ipc_addr->sa_family = 0;
      ipc_name = ipc_addr->sa_data;
      strcpy (ipc_name, s);
      strcat (ipc_name, IPC_PIPE_NAME);
      ipc_addr_len = strlen (ipc_name) + 3;
    }
  }
  return ipc_addr_len;
}


static int sock = -1;

static int
ipc_is_open P1H(void)
{
   return sock >= 0;
}


static void
ipc_open_out P1H(void) {
#ifdef IPC_DEBUG
  fputs ("tex: Opening socket for IPC output ...\n", stderr);
#endif
  if (sock >= 0) {
    return;
  }

  if (ipc_make_name () < 0) {
    sock = -1;
    return;
  }

  sock = socket (PF_UNIX, SOCK_STREAM, 0);
  if (sock >= 0) {
    if (connect (sock, ipc_addr, ipc_addr_len) != 0
        || fcntl (sock, F_SETFL, O_NONBLOCK) < 0) {
      close (sock);
      sock = -1;
      return;
    }
#ifdef IPC_DEBUG
    fputs ("tex: Successfully opened IPC socket.\n", stderr);
#endif
  }
}


static void
ipc_close_out P1H(void)
{
#ifdef IPC_DEBUG
  fputs ("tex: Closing output socket ...\n", stderr);
#endif
  if (ipc_is_open ()) {
    close (sock);
    sock = -1;
  }
}


static void
ipc_snd P3C(int, n,  int, is_eof,  char *, data)
{
  struct
  {
    struct msg msg;
    char more_data[1024];
  } ourmsg;

#ifdef IPC_DEBUG
  fputs ("tex: Sending message to socket ...\n", stderr);
#endif
  if (!ipc_is_open ()) {
    return;
  }

  ourmsg.msg.namelength = n;
  ourmsg.msg.eof = is_eof;
  if (n) {
    strcpy (ourmsg.more_data, data);
  }
  n += sizeof (struct msg);
#ifdef IPC_DEBUG
  fputs ("tex: Writing to socket...\n", stderr);
#endif
  if (write (sock, &ourmsg, n) != n) {
    ipc_close_out ();
  }
#ifdef IPC_DEBUG
  fputs ("tex: IPC message sent.\n", stderr);
#endif
}


/* This routine notifies the server if there is an eof, or the filename
   if a new DVI file is starting.  This is the routine called by TeX.
   Omega defines str_start(#) as str_start_ar(# - biggest_char), with
   biggest_char = 65535 ([4.38], [1.12] om16bit.c).  */

void
ipcpage P1C(int, is_eof)
{
  static boolean begun = false;
  unsigned len = 0;
  string p = "";

  if (!begun) {
    string name; /* Just the filename.  */
    string cwd = xgetcwd ();
    
    ipc_open_out ();
#ifndef Omega
    len = strstart[outputfilename + 1] - strstart[outputfilename];
#else
    len = strstartar[outputfilename + 1 - 65535L] -
            strstartar[outputfilename - 65535L];
#endif
    name = xmalloc (len + 1);
#ifndef Omega
    strncpy (name, &strpool[strstart[outputfilename]], len);
#else
    strncpy (name, &strpool[strstartar[outputfilename - 65535L]], len);
#endif
    name[len] = 0;
    
    /* Have to pass whole filename to the other end, since it may have
       been started up and running as a daemon, e.g., as with the NeXT
       preview program.  */
    p = concat3 (cwd, DIR_SEP_STRING, name);
    free (name);
    len = strlen(p);
    begun = true;
  }
  ipc_snd (len, is_eof, p);
  
  if (len > 0) {
    free (p);
  }
}
#endif /* TeX && IPC */

#if 0 /* TCX files are probably a bad idea, since they make TeX source
         documents unportable.  Try the inputenc LaTeX package.  */
#ifdef TeX
#ifndef Omega  /* TCX and Omega get along like sparks and gunpowder. */
/* The filename for dynamic character translation, or NULL.  */
static string translate_filename;

/* Return the next number following START, setting POST to the following
   character, as in strtol.  Issue a warning and return -1 if no number
   can be parsed.  */

static int
tcx_get_num P3C(unsigned, line_count,  string, start,  string *, post)
{
  int num = strtol (start, post, 0);
  assert (post && *post);
  if (*post == start) {
    /* Could not get a number. If blank line, fine. Else complain.  */
    string p = start;
    while (*p && ISSPACE (*p))
      p++;
    if (*p != 0)
      fprintf (stderr, "%s:%d: Expected numeric constant, not `%s'.\n",
               translate_filename, line_count, start);
    num = -1;
  } else if (num < 0 || num > 255) {
    fprintf (stderr, "%s:%d: Destination charcode %d <0 or >255.\n",
             translate_filename, line_count, num);
    num = -1;
  }  

  return num;
}


/* Look for the character translation file FNAME along the same path as
   tex.pool.  If no suffix in FNAME, use .tcx (don't bother trying to
   support extension-less names for these files).  */

static void
read_char_translation_file P1H(void)
{
  string orig_filename;
  if (!find_suffix (translate_filename)) {
    translate_filename = concat (translate_filename, ".tcx");
  }
  orig_filename = translate_filename;
  translate_filename
    = kpse_find_file (translate_filename, kpse_texpool_format, true);
  if (translate_filename) {
    string line;
    unsigned line_count = 0;
    FILE *translate_file = xfopen (translate_filename, FOPEN_R_MODE);
    while (line = read_line (translate_file)) {
      int first;
      string start2;
      string comment_loc = strchr (line, '%');
      if (comment_loc)
        *comment_loc = 0;

      line_count++;

      first = tcx_get_num (line_count, line, &start2);
      if (first >= 0) {
        string extra;
        int second;
        
        /* I suppose we could check for nonempty junk following the second
           charcode, but let's not bother.  */
        second = tcx_get_num (line_count, start2, &extra);
        if (second >= 0) {
          /* If they mention a second code, make that the internal number.  */
          xord[first] = second;
          xchr[second] = first;
        } else {
          second = first; /* else make internal the same as external */
        }

        /* Don't be confused if they happen to mention the same
           character twice.  */
        if (!isprintable[second]) {
          /* If they mention a charcode, call it printable.  */
          isprintable[second] = 1;

          /* Before, the string pool contained either three or four
             bytes as the representation for this character, e.g., `^^@'
             or `^^ff'.  But by the time we're done it will contain just
             one, the character itself.  */
          charssavedbycharset += second < 128 ? 2 : 3;
	}        
      }
      free (line);
    }
  } else {
    WARNING1 ("Could not open char translation file `%s'", orig_filename);
  }
}

/* Set up the xchr, xord, and is_printable arrays for TeX, allowing a
   translation table specified at runtime via an external file.  By
   default, no characters are translated (all 256 simply map to
   themselves) and only printable ASCII is_printable.  We must
   initialize xord at the same time as xchr, and not use the
   ``system-independent'' code in tex.web, because we want
   settings in the tcx file to override the defaults, and not simply
   assign everything in numeric order.  */

void
setupcharset P1H(void)
{
  unsigned c;
  
  /* Set up defaults.  Doing this first means the tcx file doesn't have
     to boringly specify all the usual stuff.  It also means it can't
     override the usual stuff.  This is a good thing, because of the way
     we handle the string pool. */
    for (c = 0; c <= 255; c++) {
      xchr[c] = xord[c] = c;
      isprintable[c] = (32 <= c && c <= 126);
    }
  
  /* We cannot allow the user to change the string table that is dumped
     in a .fmt file, because our strategy for dynamic translation
     assumes the dynamic changes can only decrease the number of
     characters used.  See tex.ch.  */
  if (!iniversion) {
    /* Get value from cnf file/environment variable unless already
       specified via an option.  */
    if (!translate_filename) {
      translate_filename = kpse_var_value ("TEXCHARTRANSLATE");
    }
    /* The expansion is nonempty if the variable is set.  */
    if (translate_filename) {
      read_char_translation_file ();
    }
  }
}  
#endif /* !Omega */
#endif /* TeX [character translation] */
#endif /* 0, no TCX files, please */

/* Reading the options.  */

/* Test whether getopt found an option ``A''.
   Assumes the option index is in the variable `option_index', and the
   option table in a variable `long_options'.  */
#define ARGUMENT_IS(a) STREQ (long_options[option_index].name, a)

/* SunOS cc can't initialize automatic structs, so make this static.  */
static struct option long_options[]
  = { { DUMP_OPTION,		1, 0, 0 },
      { "help",                 0, 0, 0 },
      { "ini",			0, (int *) &iniversion, 1 },
      { "interaction",          1, 0, 0 },
      { "kpathsea-debug",	1, 0, 0 },
      { "progname",             1, 0, 0 },
      { "version",              0, 0, 0 },
#ifdef TeX
#ifdef IPC
      { "ipc",			0, (int *) &ipcon, 1 },
      { "ipc-start",		0, (int *) &ipcon, 2 },
#endif /* IPC */
#ifndef Omega
      { "mltex",		0, (int *) &mltexp, 1 },
#endif /* !Omega */
      { "output-comment",	1, 0, 0 },
      { "shell-escape",		0, (int *) &shellenabledp, 1 },
#if 0 /* TCX files are probably a bad idea */
#ifndef Omega
#ifdef NONASCII
      { "translate-file",	1, 0, 0 },
#endif /* NONASCII */
#endif /* !Omega */
#endif /* 0 */
#endif /* TeX */
#if defined (TeX) || defined (MF)
      { "mktex",                1, 0, 0 },
      { "no-mktex",             1, 0, 0 },
#endif /* TeX or MF */
#ifdef MP
      { "T",			0, (int *) &troffmode, 1 },
      { "troff",		0, (int *) &troffmode, 1 },
#endif /* MP */
      { 0, 0, 0, 0 } };


static void
parse_options P2C(int, argc,  string *, argv)
{
  int g;   /* `getopt' return code.  */
  int option_index;

  for (;;) {
    g = getopt_long_only (argc, argv, "", long_options, &option_index);

    if (g == -1) /* End of arguments, exit the loop.  */
      break;

    if (g == '?') { /* Unknown option.  */
      usage (1, argv[0]);
    }

    assert (g == 0); /* We have no short option names.  */

    if (ARGUMENT_IS ("kpathsea-debug")) {
      kpathsea_debug |= atoi (optarg);

    } else if (ARGUMENT_IS ("progname")) {
      user_progname = optarg;

    } else if (ARGUMENT_IS (DUMP_OPTION)) {
      dump_name = user_progname = optarg;
      dumpoption = true;

#ifdef TeX
    } else if (ARGUMENT_IS ("output-comment")) {
      unsigned len = strlen (optarg);
      if (len < 256) {
        outputcomment = optarg;
      } else {
        WARNING2 ("Comment truncated to 255 characters from %d. (%s)",
                  len, optarg);
        outputcomment = xmalloc (256);
        strncpy (outputcomment, optarg, 255);
        outputcomment[255] = 0;
      }

#if 0 /* TCX files are probably a bad idea.  */
    } else if (ARGUMENT_IS ("translate-file")) {
      translate_filename = optarg;
#endif

#ifdef IPC
    } else if (ARGUMENT_IS ("ipc-start")) {
      ipc_open_out ();
      /* Try to start up the other end if it's not already.  */
      if (!ipc_is_open ()) {
        if (system (IPC_SERVER_CMD) == 0) {
          unsigned i;
          for (i = 0; i < 20 && !ipc_is_open (); i++) {
            sleep (2);
            ipc_open_out ();
          }
        }
     }
#endif /* IPC */
#endif /* TeX */

#if defined (TeX) || defined (MF)
    } else if (ARGUMENT_IS ("mktex")) {
      kpse_maketex_option (optarg, true);

    } else if (ARGUMENT_IS ("no-mktex")) {
      kpse_maketex_option (optarg, false);
#endif /* TeX or MF */

    } else if (ARGUMENT_IS ("interaction")) {
        /* These numbers match @d's in *.ch */
      if (STREQ (optarg, "batchmode")) {
        interactionoption = 0;
      } else if (STREQ (optarg, "nonstopmode")) {
        interactionoption = 1;
      } else if (STREQ (optarg, "scrollmode")) {
        interactionoption = 2;
      } else if (STREQ (optarg, "errorstopmode")) {
        interactionoption = 3;
      } else {
        WARNING1 ("Ignoring unknown argument `%s' to --interaction", optarg);
      }
      
    } else if (ARGUMENT_IS ("help")) {
      string help = PROGRAM_HELP;
#if defined (TeX) && defined (IPC)
      /* Don't say the options exist unless they really do.  */
      help = concat (help, TEX_IPC_HELP);
#endif
      usage (0, help);

    } else if (ARGUMENT_IS ("version")) {
      printversionandexit (BANNER, COPYRIGHT_HOLDER, AUTHOR);

    } /* Else it was a flag; getopt has already done the assignment.  */
  }
}


/* If the first thing on the command line (we use the globals `argv' and
   `optind') is a normal filename (i.e., does not start with `&' or
   `\'), and if we can open it, and if its first line is %&FORMAT, and
   FORMAT is a readable dump file, then set DUMP_VAR to FORMAT.
   Also call kpse_reset_program_name to ensure the correct paths for the
   format are used.  */

static void
maybe_set_dump_default_from_input P1C(string *, dump_var)
{
  char first_char = optind < argc ? argv[optind][0] : 0;
  
  if (first_char != '&' && first_char != '\\' && first_char) {
    /* If the file can't be found, don't look too hard now.  We'll
       detect that it's missing in the normal course of things and give
       the error then.  */
    string in_name = kpse_find_file (argv[optind], INPUT_FORMAT, false);
    FILE *f = in_name ? fopen (in_name, FOPEN_R_MODE) : NULL;
    if (f) {
      string first_line = read_line (f);
      xfclose (f, in_name);
      
      /* %&ini is special.  */
      if (first_line && first_line[0] == '%' && first_line[1] == '&') {
        if (strncmp (first_line + 1, "ini", 3) == 0) {
          iniversion = true;
        } else {
          string f_name = concat (first_line+2, DUMP_EXT);
          string d_name = kpse_find_file (f_name, DUMP_FORMAT, false);
          if (d_name && kpse_readable_file (d_name)) {
            dump_name = xstrdup (first_line + 2);
            kpse_reset_program_name (dump_name);
            /* Tell TeX/MF/MP we have a %&name line... */
            dumpline = true;
          }
          free (f_name);
        }
      }
      
      if (first_line)
        free (first_line);
    }
  }
}

#if defined (TeX) || defined (MP)
/* Return true if FNAME is acceptable as a name for \openout.  */

boolean
openoutnameok P1C(const_string, fname)
{
  /* We distinguish three cases:
     'a' (any)        allows any file to be opened for writing.
     'r' (restricted) means disallowing special file names.
     'p' (paranoid)   means being really paranoid: disallowing special file
                      names and restricting output files to be in or below
                      the working directory or $TEXMFOUTPUT.
     We default to "paranoid".
     This function contains several return statements...  */

  const_string openout_any = kpse_var_value ("openout_any");

  if (openout_any && (*openout_any == 'a'
                      || *openout_any == 'y'
                      || *openout_any == '1'))
    return true;

#if defined (unix) && !defined (MSDOS)
  {
    const_string base = basename (fname);
    /* Disallow .rhosts, .login, etc.  Allow .tex (for LaTeX).  */
    if (*base == 0 || (*base == '.' && !STREQ (base, ".tex")))
      return false;
  }
#else
  /* Other OSs don't have special names? */
#endif

  if (openout_any && (*openout_any == 'r'
                      || *openout_any == 'n'
                      || *openout_any == '0'))
    return true;

  /* Paranoia supplied by Charles Karney...  */
  if (kpse_absolute_p (fname, false)) {
    const_string texmfoutput = kpse_var_value ("TEXMFOUTPUT");
    /* Absolute pathname is only OK if TEXMFOUTPUT is set, it's not empty,
       fname begins the TEXMFOUTPUT, and is followed by / */
    if (!texmfoutput || *texmfoutput == '\0'
        || fname != strstr (fname, texmfoutput)
        || fname[strlen(texmfoutput)] != DIR_SEP)
      return false;
  }
  /* For all pathnames, we disallow "../" at the beginning or "/../"
     anywhere.  */
  if (fname[0] == '.' && fname[1] == '.' && fname[2] == DIR_SEP) {
    return false;
  } else {
    const_string dotpair = strstr (fname, "..");
    /* If dotpair[2] == DIR_SEP, then dotpair[-1] is well-defined. */
    if (dotpair && dotpair[2] == DIR_SEP && dotpair[-1] == DIR_SEP)
      return false;
  }

  /* We passed all tests.  */
  return true;
}
#endif /* TeX or MP */

/* All our interrupt handler has to do is set TeX's or Metafont's global
   variable `interrupt'; then they will do everything needed.  */
#ifdef WIN32
/* Win32 doesn't set SIGINT ... */
BOOL WINAPI
catch_interrupt (DWORD arg)
{
  switch (arg) {
  case CTRL_C_EVENT:
  case CTRL_BREAK_EVENT:
    interrupt = 1;
    return TRUE;
  default:
    /* No need to set interrupt as we are exiting anyway */
    return FALSE;
  }
}
#else /* not WIN32 */
static RETSIGTYPE
catch_interrupt P1C (int, arg)
{
  interrupt = 1;
#ifdef OS2
  (void) signal (SIGINT, SIG_ACK);
#else
  (void) signal (SIGINT, catch_interrupt);
#endif /* not OS2 */
}
#endif /* not WIN32 */

/* Besides getting the date and time here, we also set up the interrupt
   handler, for no particularly good reason.  It's just that since the
   `fix_date_and_time' routine is called early on (section 1337 in TeX,
   ``Get the first line of input and prepare to start''), this is as
   good a place as any.  */

void
get_date_and_time P4C(integer *, minutes,  integer *, day,
                      integer *, month,  integer *, year)
{
  time_t clock = time ((time_t *) 0);
  struct tm *tmptr = localtime (&clock);

  *minutes = tmptr->tm_hour * 60 + tmptr->tm_min;
  *day = tmptr->tm_mday;
  *month = tmptr->tm_mon + 1;
  *year = tmptr->tm_year + 1900;

  {
#ifdef SA_INTERRUPT
    /* Under SunOS 4.1.x, the default action after return from the
       signal handler is to restart the I/O if nothing has been
       transferred.  The effect on TeX is that interrupts are ignored if
       we are waiting for input.  The following tells the system to
       return EINTR from read() in this case.  From ken@cs.toronto.edu.  */

    struct sigaction a, oa;

    a.sa_handler = catch_interrupt;
    sigemptyset (&a.sa_mask);
    sigaddset (&a.sa_mask, SIGINT);
    a.sa_flags = SA_INTERRUPT;
    sigaction (SIGINT, &a, &oa);
    if (oa.sa_handler != SIG_DFL)
      sigaction (SIGINT, &oa, (struct sigaction *) 0);
#else /* no SA_INTERRUPT */
#ifdef WIN32
    SetConsoleCtrlHandler(catch_interrupt, TRUE);
#else /* not WIN32 */
    RETSIGTYPE (*old_handler) P1H(int);
    
    old_handler = signal (SIGINT, catch_interrupt);
    if (old_handler != SIG_DFL)
      signal (SIGINT, old_handler);
#endif /* not WIN32 */
#endif /* no SA_INTERRUPT */
  }
}

/* Read a line of input as efficiently as possible while still looking
   like Pascal.  We set `last' to `first' and return `false' if we get
   to eof.  Otherwise, we return `true' and set last = first +
   length(line except trailing whitespace).  */

boolean
input_line P1C(FILE *, f)
{
  int i;

  /* Recognize either LF or CR as a line terminator.  */
  last = first;
  while (last < bufsize && (i = getc (f)) != EOF && i != '\n' && i != '\r')
    buffer[last++] = i;

  if (i == EOF && last == first)
    return false;

  /* We didn't get the whole line because our buffer was too small.  */
  if (i != EOF && i != '\n' && i != '\r') {
    fprintf (stderr, "! Unable to read an entire line---bufsize=%u.\n",
                     (unsigned) bufsize);
    fputs ("Please increase buf_size in texmf.cnf.\n", stderr);
    uexit (1);
  }

  buffer[last] = ' ';
  if (last >= maxbufstack)
    maxbufstack = last;

  /* If next char is LF of a CRLF, read it.  */
  if (i == '\r') {
    i = getc (f);
    if (i != '\n')
      ungetc (i, f);
  }
  
  /* Trim trailing whitespace.  */
  while (last > first && isblank (buffer[last - 1]))
    --last;

  /* Don't bother using xord if we don't need to.  */
#ifndef Omega
#ifdef NONASCII
  for (i = first; i <= last; i++)
     buffer[i] = xord[buffer[i]];
#endif
#endif

    return true;
}

/* This string specifies what the `e' option does in response to an
   error message.  */ 
static char *edit_value = EDITOR;

/* This procedure originally due to sjc@s1-c.  TeX & Metafont call it when
   the user types `e' in response to an error, invoking a text editor on
   the erroneous source file.  FNSTART is how far into FILENAME the
   actual filename starts; FNLENGTH is how long the filename is.  */
   
void
calledit (filename, fnstart, fnlength, linenumber)
    ASCIIcode *filename;
    poolpointer fnstart;
    integer fnlength, linenumber;
{
  char *temp, *command;
  char c;
  int sdone, ddone, i;

  sdone = ddone = 0;
  filename += fnstart;

  /* Close any open input files, since we're going to kill the job.  */
  for (i = 1; i <= inopen; i++)
    xfclose (inputfile[i], "inputfile");

  /* Replace the default with the value of the appropriate environment
     variable or config file value, if it's set.  */
  temp = kpse_var_value (edit_var);
  if (temp != NULL)
    edit_value = temp;

  /* Construct the command string.  The `11' is the maximum length an
     integer might be.  */
  command = (string) xmalloc (strlen (edit_value) + fnlength + 11);

  /* So we can construct it as we go.  */
  temp = command;

  while ((c = *edit_value++) != 0)
    {
      if (c == '%')
        {
          switch (c = *edit_value++)
            {
	    case 'd':
	      if (ddone)
                FATAL ("call_edit: `%%d' appears twice in editor command");
              sprintf (temp, "%ld", linenumber);
              while (*temp != '\0')
                temp++;
              ddone = 1;
              break;

	    case 's':
              if (sdone)
                FATAL ("call_edit: `%%s' appears twice in editor command");
              for (i =0; i < fnlength; i++)
		*temp++ = Xchr (filename[i]);
              sdone = 1;
              break;

	    case '\0':
              *temp++ = '%';
              /* Back up to the null to force termination.  */
	      edit_value--;
	      break;

	    default:
	      *temp++ = '%';
	      *temp++ = c;
	      break;
	    }
	}
      else
	*temp++ = c;
    }

  *temp = 0;

  /* Execute the command.  */
  if (system (command) != 0)
    fprintf (stderr, "! Trouble executing `%s'.\n", command);

  /* Quit, since we found an error.  */
  uexit (1);
}

/* Read and write dump files.  As distributed, these files are
   architecture dependent; specifically, BigEndian and LittleEndian
   architectures produce different files.  These routines always output
   BigEndian files.  This still does not guarantee them to be
   architecture-independent, because it is possible to make a format
   that dumps a glue ratio, i.e., a floating-point number.  Fortunately,
   none of the standard formats do that.  */

#if !defined (WORDS_BIGENDIAN) && !defined (NO_DUMP_SHARE) /* this fn */

/* This macro is always invoked as a statement.  It assumes a variable
   `temp'.  */
   
#define SWAP(x, y) temp = (x); (x) = (y); (y) = temp


/* Make the NITEMS items pointed at by P, each of size SIZE, be the
   opposite-endianness of whatever they are now.  */

static void
swap_items P3C(char *, p,  int, nitems,  int, size)
{
  char temp;

  /* Since `size' does not change, we can write a while loop for each
     case, and avoid testing `size' for each time.  */
  switch (size)
    {
    /* 16-byte items happen on the DEC Alpha machine when we are not
       doing sharable memory dumps.  */
    case 16:
      while (nitems--)
        {
          SWAP (p[0], p[15]);
          SWAP (p[1], p[14]);
          SWAP (p[2], p[13]);
          SWAP (p[3], p[12]);
          SWAP (p[4], p[11]);
          SWAP (p[5], p[10]);
          SWAP (p[6], p[9]);
          SWAP (p[7], p[8]);
          p += size;
        }
      break;

    case 8:
      while (nitems--)
        {
          SWAP (p[0], p[7]);
          SWAP (p[1], p[6]);
          SWAP (p[2], p[5]);
          SWAP (p[3], p[4]);
          p += size;
        }
      break;

    case 4:
      while (nitems--)
        {
          SWAP (p[0], p[3]);
          SWAP (p[1], p[2]);
          p += size;
        }
      break;

    case 2:
      while (nitems--)
        {
          SWAP (p[0], p[1]);
          p += size;
        }
      break;

    case 1:
      /* Nothing to do.  */
      break;

    default:
      FATAL1 ("Can't swap a %d-byte item for (un)dumping", size);
  }
}
#endif /* not WORDS_BIGENDIAN and not NO_DUMP_SHARE */


/* Here we write NITEMS items, each item being ITEM_SIZE bytes long.
   The pointer to the stuff to write is P, and we write to the file
   OUT_FILE.  */

void
do_dump P4C(char *, p,  int, item_size,  int, nitems,  FILE *, out_file)
{
#if !defined (WORDS_BIGENDIAN) && !defined (NO_DUMP_SHARE)
  swap_items (p, nitems, item_size);
#endif

  if (fwrite (p, item_size, nitems, out_file) != nitems)
    {
      fprintf (stderr, "! Could not write %d %d-byte item(s).\n",
               nitems, item_size);
      uexit (1);
    }

  /* Have to restore the old contents of memory, since some of it might
     get used again.  */
#if !defined (WORDS_BIGENDIAN) && !defined (NO_DUMP_SHARE)
  swap_items (p, nitems, item_size);
#endif
}


/* Here is the dual of the writing routine.  */

void
do_undump P4C(char *, p,  int, item_size,  int, nitems,  FILE *, in_file)
{
  if (fread (p, item_size, nitems, in_file) != nitems)
    FATAL2 ("Could not undump %d %d-byte item(s)", nitems, item_size);

#if !defined (WORDS_BIGENDIAN) && !defined (NO_DUMP_SHARE)
  swap_items (p, nitems, item_size);
#endif
}

/* Look up VAR_NAME in texmf.cnf; assign either the value found there or
   DFLT to *VAR.  */

void
setupboundvariable P3C(integer *, var,  const_string, var_name,  integer, dflt)
{
  string expansion = kpse_var_value (var_name);
  *var = dflt;

  if (expansion) {
    integer conf_val = atoi (expansion);
    /* It's ok if the cnf file specifies 0 for extra_mem_{top,bot}, etc.
       But negative numbers are always wrong.  */
    if (conf_val < 0 || conf_val == 0 && dflt > 0) {
      fprintf (stderr,
               "%s: Bad value (%ld) in texmf.cnf for %s, keeping %ld.\n",
               program_invocation_name,
               (long) conf_val, var_name + 1, (long) dflt);
    } else {
      *var = conf_val; /* We'll make further checks later.  */
    }
    free (expansion);
  }
}

#ifdef MP
/* Invoke makempx (or troffmpx) to make sure there is an up-to-date
   .mpx file for a given .mp file.  (Original from John Hobby 3/14/90)  */

#include <kpathsea/concatn.h>

#ifndef MPXCOMMAND
#define MPXCOMMAND "makempx"
#endif

boolean
callmakempx P2C(string, mpname,  string, mpxname)
{
  int ret;
  string cnf_cmd = kpse_var_value ("MPXCOMMAND");
  
  if (cnf_cmd && STREQ (cnf_cmd, "0")) {
    /* If they turned off this feature, just return success.  */
    ret = 0;

  } else {
    /* We will invoke something. Compile-time default if nothing else.  */
    string cmd;
    if (!cnf_cmd)
      cnf_cmd = xstrdup (MPXCOMMAND);

    cmd = concatn (cnf_cmd, troffmode ? " -troff " : " ",
                   mpname, " ", mpxname, NULL);
    /* Run it.  */
    ret = system (cmd);
    free (cmd);
  }

  free (cnf_cmd);
  return ret == 0;
}
#endif /* MP */

/* Metafont/MetaPost fraction routines. Replaced either by assembler or C.
   The assembler syntax doesn't work on Solaris/x86.  */
#ifndef TeX
#ifdef __sun__
#define NO_MF_ASM
#endif
#if defined(WIN32) && !defined(NO_MF_ASM)
#include "lib/mfmpw32.c"
#elif defined (__i386__) && defined (__GNUC__) && !defined (NO_MF_ASM)
#include "lib/mfmpi386.asm"
#else
/* Replace fixed-point fraction routines from mf.web and mp.web with
   Hobby's floating-point C code.  */

/****************************************************************
Copyright 1990 - 1995 by AT&T Bell Laboratories.

Permission to use, copy, modify, and distribute this software
and its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the names of AT&T Bell Laboratories or
any of its entities not be used in advertising or publicity
pertaining to distribution of the software without specific,
written prior permission.

AT&T disclaims all warranties with regard to this software,
including all implied warranties of merchantability and fitness.
In no event shall AT&T be liable for any special, indirect or
consequential damages or any damages whatsoever resulting from
loss of use, data or profits, whether in an action of contract,
negligence or other tortious action, arising out of or in
connection with the use or performance of this software.
****************************************************************/

/**********************************************************
 The following is by John Hobby
 **********************************************************/

#ifndef FIXPT

/* These replacements for takefraction, makefraction, takescaled, makescaled
   run about 3 to 11 times faster than the standard versions on modern machines
   that have fast hardware for double-precision floating point.  They should
   produce approximately correct results on all machines and agree exactly
   with the standard versions on machines that satisfy the following conditions:
   1. Doubles must have at least 46 mantissa bits; i.e., numbers expressible
      as n*2^k with abs(n)<2^46 should be representable.
   2. The following should hold for addition, subtraction, and multiplcation but
      not necessarily for division:
      A. If the true answer is between two representable numbers, the computed
         answer must be one of them.
      B. When the true answer is representable, this must be the computed result.
   3. Dividing one double by another should always produce a relative error of
      at most one part in 2^46.  (This is why the mantissa requirement is
      46 bits instead of 45 bits.)
   3. In the absence of overflow, double-to-integer conversion should truncate
      toward zero and do this in an exact fashion.
   4. Integer-to-double convesion should produce exact results.
   5. Dividing one power of two by another should yield an exact result.
   6. ASCII to double conversion should be exact for integer values.
   7. Integer arithmetic must be done in the two's-complement system.
*/
#define ELGORDO  0x7fffffff
#define TWEXP31  2147483648.0
#define TWEXP28  268435456.0
#define TWEXP16 65536.0
#define TWEXP_16 (1.0/65536.0)
#define TWEXP_28 (1.0/268435456.0)

integer
ztakefraction(p,q)		/* Approximate p*q/2^28 */
integer p,q;
{	register double d;
	register integer i;
	d = (double)p * (double)q * TWEXP_28;
	if ((p^q) >= 0) {
		d += 0.5;
		if (d>=TWEXP31) {
			if (d!=TWEXP31 || (((p&077777)*(q&077777))&040000)==0)
				aritherror = true;
			return ELGORDO;
		}
		i = (integer) d;
		if (d==i && (((p&077777)*(q&077777))&040000)!=0) --i;
	} else {
		d -= 0.5;
		if (d<= -TWEXP31) {
			if (d!= -TWEXP31 || ((-(p&077777)*(q&077777))&040000)==0)
				aritherror = true;
			return -ELGORDO;
		}
		i = (integer) d;
		if (d==i && ((-(p&077777)*(q&077777))&040000)!=0) ++i;
	}
	return i;
}

integer
ztakescaled(p,q)		/* Approximate p*q/2^16 */
integer p,q;
{	register double d;
	register integer i;
	d = (double)p * (double)q * TWEXP_16;
	if ((p^q) >= 0) {
		d += 0.5;
		if (d>=TWEXP31) {
			if (d!=TWEXP31 || (((p&077777)*(q&077777))&040000)==0)
				aritherror = true;
			return ELGORDO;
		}
		i = (integer) d;
		if (d==i && (((p&077777)*(q&077777))&040000)!=0) --i;
	} else {
		d -= 0.5;
		if (d<= -TWEXP31) {
			if (d!= -TWEXP31 || ((-(p&077777)*(q&077777))&040000)==0)
				aritherror = true;
			return -ELGORDO;
		}
		i = (integer) d;
		if (d==i && ((-(p&077777)*(q&077777))&040000)!=0) ++i;
	}
	return i;
}

/* Note that d cannot exactly equal TWEXP31 when the overflow test is made
   because the exact value of p/q cannot be strictly between (2^31-1)/2^28
   and 8/1.  No pair of integers less than 2^31 has such a ratio.
*/
integer
zmakefraction(p,q)		/* Approximate 2^28*p/q */
integer p,q;
{	register double d;
	register integer i;
#ifdef DEBUG
	if (q==0) confusion(47); 
#endif /* DEBUG */
	d = TWEXP28 * (double)p /(double)q;
	if ((p^q) >= 0) {
		d += 0.5;
		if (d>=TWEXP31) {aritherror=true; return ELGORDO;}
		i = (integer) d;
		if (d==i && ( ((q>0 ? -q : q)&077777)
				* (((i&037777)<<1)-1) & 04000)!=0) --i;
	} else {
		d -= 0.5;
		if (d<= -TWEXP31) {aritherror=true; return -ELGORDO;}
		i = (integer) d;
		if (d==i && ( ((q>0 ? q : -q)&077777)
				* (((i&037777)<<1)+1) & 04000)!=0) ++i;
	}
	return i;
}

/* Note that d cannot exactly equal TWEXP31 when the overflow test is made
   because the exact value of p/q cannot be strictly between (2^31-1)/2^16
   and 2^15/1.  No pair of integers less than 2^31 has such a ratio.
*/
integer
zmakescaled(p,q)		/* Approximate 2^16*p/q */
integer p,q;
{	register double d;
	register integer i;
#ifdef DEBUG
	if (q==0) confusion(47); 
#endif /* DEBUG */
	d = TWEXP16 * (double)p /(double)q;
	if ((p^q) >= 0) {
		d += 0.5;
		if (d>=TWEXP31) {aritherror=true; return ELGORDO;}
		i = (integer) d;
		if (d==i && ( ((q>0 ? -q : q)&077777)
				* (((i&037777)<<1)-1) & 04000)!=0) --i;
	} else {
		d -= 0.5;
		if (d<= -TWEXP31) {aritherror=true; return -ELGORDO;}
		i = (integer) d;
		if (d==i && ( ((q>0 ? q : -q)&077777)
				* (((i&037777)<<1)+1) & 04000)!=0) ++i;
	}
	return i;
}

#endif /* not FIXPT */
#endif /* not assembler */
#endif /* not TeX, i.e., MF or MP */

#ifdef MF
/* On-line display routines for Metafont.  Here we use a dispatch table
   indexed by the MFTERM or TERM environment variable to select the
   graphics routines appropriate to the user's terminal.  stdout must be
   connected to a terminal for us to do any graphics.  */

#ifdef AMIGAWIN
extern int mf_amiga_initscreen P1H(void);
extern void mf_amiga_updatescreen P1H(void);
extern void mf_amiga_blankrectangle P4H(screencol, screencol, screenrow, screenrow);
extern void mf_amiga_paintrow P4H(screenrow, pixelcolor, transspec, screencol);
#endif
#ifdef EPSFWIN
extern int mf_epsf_initscreen P1H(void);
extern void mf_epsf_updatescreen P1H(void);
extern void mf_epsf_blankrectangle P4H(screencol, screencol, screenrow, screenrow);
extern void mf_epsf_paintrow P4H(screenrow, pixelcolor, transspec, screencol);
#endif
#ifdef HP2627WIN
extern int mf_hp2627_initscreen P1H(void);
extern void mf_hp2627_updatescreen P1H(void);
extern void mf_hp2627_blankrectangle P4H(screencol, screencol, screenrow, screenrow);
extern void mf_hp2627_paintrow P4H(screenrow, pixelcolor, transspec, screencol);
#endif
#ifdef MFTALKWIN
extern int mf_mftalk_initscreen P1H(void);
extern void mf_mftalk_updatescreen P1H(void);
extern void mf_mftalk_blankrectangle P4H(screencol, screencol, screenrow, screenrow);
extern void mf_mftalk_paintrow P4H(screenrow, pixelcolor, transspec, screencol);
#endif
#ifdef NEXTWIN
extern int mf_next_initscreen P1H(void);
extern void mf_next_updatescreen P1H(void);
extern void mf_next_blankrectangle P4H(screencol, screencol, screenrow, screenrow);
extern void mf_next_paintrow P4H(screenrow, pixelcolor, transspec, screencol);
#endif
#ifdef REGISWIN
extern int mf_regis_initscreen P1H(void);
extern void mf_regis_updatescreen P1H(void);
extern void mf_regis_blankrectangle P4H(screencol, screencol, screenrow, screenrow);
extern void mf_regis_paintrow P4H(screenrow, pixelcolor, transspec, screencol);
#endif
#ifdef SUNWIN
extern int mf_sun_initscreen P1H(void);
extern void mf_sun_updatescreen P1H(void);
extern void mf_sun_blankrectangle P4H(screencol, screencol, screenrow, screenrow);
extern void mf_sun_paintrow P4H(screenrow, pixelcolor, transspec, screencol);
#endif
#ifdef TEKTRONIXWIN
extern int mf_tektronix_initscreen P1H(void);
extern void mf_tektronix_updatescreen P1H(void);
extern void mf_tektronix_blankrectangle P4H(screencol, screencol, screenrow, screenrow);
extern void mf_tektronix_paintrow P4H(screenrow, pixelcolor, transspec, screencol);
#endif
#ifdef UNITERMWIN
extern int mf_uniterm_initscreen P1H(void);
extern void mf_uniterm_updatescreen P1H(void);
extern void mf_uniterm_blankrectangle P4H(screencol, screencol, screenrow, screenrow);
extern void mf_uniterm_paintrow P4H(screenrow, pixelcolor, transspec, screencol);
#endif
#ifdef WIN32WIN
extern int mf_win32_initscreen P1H(void);
extern void mf_win32_updatescreen P1H(void);
extern void mf_win32_blankrectangle P4H(screencol, screencol, screenrow, screenrow);
extern void mf_win32_paintrow P4H(screenrow, pixelcolor, transspec, screencol);
#endif
#ifdef X11WIN
extern int mf_x11_initscreen P1H(void);
extern void mf_x11_updatescreen P1H(void);
extern void mf_x11_blankrectangle P4H(screencol, screencol, screenrow, screenrow);
extern void mf_x11_paintrow P4H(screenrow, pixelcolor, transspec, screencol);
#endif
extern int mf_trap_initscreen P1H(void);
extern void mf_trap_updatescreen P1H(void);
extern void mf_trap_blankrectangle P4H(screencol, screencol, screenrow, screenrow);
extern void mf_trap_paintrow P4H(screenrow, pixelcolor, transspec, screencol);


/* This variable, `mfwsw', contains the dispatch tables for each
   terminal.  We map the Pascal calls to the routines `init_screen',
   `update_screen', `blank_rectangle', and `paint_row' into the
   appropriate entry point for the specific terminal that MF is being
   run on.  */

struct mfwin_sw
{
  char *mfwsw_type;		/* Name of terminal a la TERMCAP.  */
  int (*mfwsw_initscreen) P1H(void);
  void (*mfwsw_updatescrn) P1H(void);
  void (*mfwsw_blankrect) P4H(screencol, screencol, screenrow, screenrow);
  void (*mfwsw_paintrow) P4H(screenrow, pixelcolor, transspec, screencol);
} mfwsw[] =
{
#ifdef AMIGAWIN
  { "amiterm", mf_amiga_initscreen, mf_amiga_updatescreen,
    mf_amiga_blankrectangle, mf_amiga_paintrow },
#endif
#ifdef EPSFWIN
  { "epsf", mf_epsf_initscreen, mf_epsf_updatescreen, 
    mf_epsf_blankrectangle, mf_epsf_paintrow },
#endif
#ifdef HP2627WIN
  { "hp2627", mf_hp2627_initscreen, mf_hp2627_updatescreen,
    mf_hp2627_blankrectangle, mf_hp2627_paintrow },
#endif
#ifdef MFTALKWIN
  { "mftalk", mf_mftalk_initscreen, mf_mftalk_updatescreen, 
     mf_mftalk_blankrectangle, mf_mftalk_paintrow },
#endif
#ifdef NEXTWIN
  { "next", mf_next_initscreen, mf_next_updatescreen,
    mf_next_blankrectangle, mf_next_paintrow },
#endif
#ifdef REGISWIN
  { "regis", mf_regis_initscreen, mf_regis_updatescreen,
    mf_regis_blankrectangle, mf_regis_paintrow },
#endif
#ifdef SUNWIN
  { "sun", mf_sun_initscreen, mf_sun_updatescreen,
    mf_sun_blankrectangle, mf_sun_paintrow },
#endif
#ifdef TEKTRONIXWIN
  { "tek", mf_tektronix_initscreen, mf_tektronix_updatescreen,
    mf_tektronix_blankrectangle, mf_tektronix_paintrow },
#endif
#ifdef UNITERMWIN
   { "uniterm", mf_uniterm_initscreen, mf_uniterm_updatescreen,
     mf_uniterm_blankrectangle, mf_uniterm_paintrow },
#endif
#ifdef WIN32WIN
  { "win32term", mf_win32_initscreen, mf_win32_updatescreen, 
    mf_win32_blankrectangle, mf_win32_paintrow },
#endif
#ifdef X11WIN
  { "xterm", mf_x11_initscreen, mf_x11_updatescreen, 
    mf_x11_blankrectangle, mf_x11_paintrow },
#endif
  
  /* Always support this.  */
  { "trap", mf_trap_initscreen, mf_trap_updatescreen,
    mf_trap_blankrectangle, mf_trap_paintrow },

/* Finally, we must have an entry with a terminal type of NULL.  */
  { NULL, NULL, NULL, NULL, NULL }

}; /* End of the array initialization.  */


/* This is a pointer to the mfwsw[] entry that we find.  */
static struct mfwin_sw *mfwp;


/* The following are routines that just jump to the correct
   terminal-specific graphics code. If none of the routines in the
   dispatch table exist, or they fail, we produce trap-compatible
   output, i.e., the same words and punctuation that the unchanged
   mf.web would produce.  */


/* This returns true if we can do window operations, else false.  */

boolean
initscreen P1H(void)
{
  /* If MFTERM is set, use it.  */
  char *tty_type = kpse_var_value ("MFTERM");
  
  if (tty_type == NULL)
    { 
#if defined (AMIGA)
      tty_type = "amiterm";
#elif defined (WIN32)
      tty_type = "win32term";
#elif defined (OS2) || defined (__DJGPP__) /* not AMIGA nor WIN32 */
      tty_type = "mftalk";
#else /* not (OS2 or WIN32 or __DJGPP__ or AMIGA) */
      /* If DISPLAY is set, we are X11; otherwise, who knows.  */
      boolean have_display = getenv ("DISPLAY") != NULL;
      tty_type = have_display ? "xterm" : getenv ("TERM");

      /* If we don't know what kind of terminal this is, or if Metafont
         isn't being run interactively, don't do any online output.  */
      if (tty_type == NULL
          || (!STREQ (tty_type, "trap") && !isatty (fileno (stdout))))
        return 0;
#endif /* not (OS2 or WIN32 or __DJGPP__ or AMIGA) */
  }

  /* Test each of the terminals given in `mfwsw' against the terminal
     type, and take the first one that matches, or if the user is running
     under Emacs, the first one.  */
  for (mfwp = mfwsw; mfwp->mfwsw_type != NULL; mfwp++) {
    if (!strncmp (mfwp->mfwsw_type, tty_type, strlen (mfwp->mfwsw_type))
	|| STREQ (tty_type, "emacs"))
      if (mfwp->mfwsw_initscreen)
	return ((*mfwp->mfwsw_initscreen) ());
      else {
        fprintf (stderr, "mf: Couldn't initialize online display for `%s'.\n",
                 tty_type);
        break;
      }
  }
  
  /* We disable X support by default, since most sites don't use it, and
     variations in X configurations seem impossible to overcome
     automatically. Too frustrating for everyone involved.  */
  if (STREQ (tty_type, "xterm")) {
    fputs ("\nmf: Window support for X was not compiled into this binary.\n",
            stderr);
  fputs ("mf: To do so, rerun configure --with-x, recompile, and reinstall.\n",
           stderr);
    fputs ("mf: (Or perhaps you just failed to specify the mode.)\n", stderr);
  }

  /* The current terminal type wasn't found in any of the entries, or
     initalization failed, so silently give up, assuming that the user
     isn't on a terminal that supports graphic output.  */
  return 0;
}


/* Make sure everything is visible.  */

void
updatescreen P1H(void)
{
  if (mfwp->mfwsw_updatescrn)
    (*mfwp->mfwsw_updatescrn) ();
}


/* This sets the rectangle bounded by ([left,right], [top,bottom]) to
   the background color.  */

void
blankrectangle (left, right, top, bottom)
    screencol left, right;
    screenrow top, bottom;
{
  if (mfwp->mfwsw_blankrect)
    (*mfwp->mfwsw_blankrect) (left, right, top, bottom);
}


/* This paints ROW, starting with the color INIT_COLOR. 
   TRANSITION_VECTOR then specifies the length of the run; then we
   switch colors.  This goes on for VECTOR_SIZE transitions.  */

void
paintrow (row, init_color, transition_vector, vector_size)
    screenrow row;
    pixelcolor init_color;
    transspec transition_vector;
    screencol vector_size;
{
  if (mfwp->mfwsw_paintrow)
    (*mfwp->mfwsw_paintrow) (row, init_color, transition_vector, vector_size);
}
#endif /* MF */

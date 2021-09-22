
#ifdef _WIN32
# ifdef mkdir
#  undef mkdir
# endif
# define mkdir(p,m)  _mkdir(p)
typedef unsigned long mode_t;
#else
# define __cdecl
# define _getdrive()		(getdisk() + 1)
# define FS_CASE_SENSITIVE	_FILESYS_CASE_SENSITIVE
typedef unsigned int DWORD;
#endif


/* gsftopk defines KPSE_LAST_DEBUG+2, so avoid clashes */
#define MKTEX_DEBUG (KPSE_LAST_DEBUG + 3)
#define MKTEX_FINE_DEBUG (KPSE_LAST_DEBUG + 4)

/*
  We are keeping trace of the environment (ie: cwd, file redirections)
  with the help of these ops and structures. There is a global stack
  inidcating wich actions have been taken.
  */
typedef enum { CHDIR = 1, REDIRECT } op_env;
typedef struct mod_env {
  op_env op;
  union {
  char *path;
  int oldfd[3];
  } data;
} mod_env;

/* from stackenv.c */
extern void __cdecl oops(const char *, ...);
#ifdef __GNUC__
extern void mt_exit(int) __attribute__((noreturn));
#else
extern void mt_exit(int);
#endif
extern void pushd(char *);
extern void popd(void);
extern char *peek_dir(int);
extern void push_fd(int [3]);
extern void pop_fd(void);
extern int  mvfile(const char *, const char *);
extern int  catfile(const char *, const char *, int);
extern string dirname(const_string);
extern boolean test_file(int, string);
extern boolean is_writable(string);
extern void start_redirection(void);
extern int execute_command(string);
#ifdef _WIN32
/* extern void mt_exit(int); */
extern BOOL sigint_handler(DWORD);
#else
extern void sigint_handler(int);
#endif

#define LAST_CHAR(s) (*((s)+strlen(s)-1))

/* core functions */
extern int getcrnl(char*, int n);
extern char* nextarg(char*);
extern int sendcrnl(char*, ...);
extern int senderr(char*, ...);
extern int sendok(char*, ...);

/* system specific */
extern void hello(void);
extern int usercmd(char*);
extern int apopcmd(char*);
extern int passcmd(char*);

extern char user[NAMELEN];
extern int loggedin;
extern String *mailfile;
extern int reverse;


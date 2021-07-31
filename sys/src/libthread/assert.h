
void _threaddebug(ulong, char *fmt, ...);
void _threadfatal(char *fmt, ...);
void _threadassert(char *s);

extern int _threaddebuglevel;

#define DBGAPPL	(1 << 0)
#define DBGTHRD	(1 << 16)
#define DBGCHAN	(1 << 17)
#define DBGREND	(1 << 18)
#define DBGKILL	(1 << 19)
#define DBGNOTE	(1 << 20)
#define DBGQUE	(1 << 21)
#define DBGCHLD	(1 << 22)
#define DBGPROC	(1 << 23)

#define	threadassert(x)	if(x);else _threadassert("x")

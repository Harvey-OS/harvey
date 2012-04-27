typedef struct Thr Thr;

enum
{
	Stack = 24 * 1024
};

enum
{
	Trun,	/* running */
	Trdy,	/* ready */
	Tblk,	/* issuing a system call */
	Tsys,	/* waiting for a system call */
	Tdead,	/* exiting */
};

struct Thr
{
	Nixcall;
	Nixret	nr;
	char*	name;
	int	state;
	int	id;
	uchar*	stk;
	jmp_buf label;
	void	(*main)(int, void *[]);
	int	argc;
	void**	argv;
	Thr*	next;	/* in list */
	Thr*	prev;	/* only in sysq */
};


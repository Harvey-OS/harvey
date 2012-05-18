typedef struct Kvalue Kvalue;
typedef struct Kexecgrp Kexecgrp;


/* Kexec structures */
struct Kvalue
{
	uintptr addr;
	uvlong size;
	int	len;
	int inuse;
	Kvalue	*link;
	Qid	qid;
};

struct Kexecgrp
{
	Ref;
	RWlock;
	Kvalue	**ent;
	int	nent;
	int	ment;
	ulong	path;	/* qid.path of next Kvalue to be allocated */
	ulong	vers;	/* of Kexecgrp */
};

void	kforkexecac(Proc*, int, char*, char**);
Proc*	setupseg(int core);

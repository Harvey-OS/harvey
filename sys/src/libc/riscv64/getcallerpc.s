#define LINK	R1
#define SP	R2
#define RARG	R8

TEXT	getcallerpc(SB), $0
	MOV	0(SP), RARG
	RET


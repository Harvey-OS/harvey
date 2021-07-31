Drive* mmcprobe(Scsi*);
char*	geterrstr(void);
Buf*	bopen(long (*)(Buf*, void*, long, long), int, int, int);
void	bterm(Buf*);
long	bread(Buf*, void*, long, long);
long	bwrite(Buf*, void*, long);
long bufread(Otrack*, void*, long, long);
long bufwrite(Otrack*, void*, long);
void *emalloc(ulong);

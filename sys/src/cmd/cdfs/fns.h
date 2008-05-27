Buf*	bopen(long (*)(Buf*, void*, long, ulong), int, int, int);
long	bread(Buf*, void*, long, vlong);
void	bterm(Buf*);
long	bufread(Otrack*, void*, long, vlong);
long	bufwrite(Otrack*, void*, long);
long	bwrite(Buf*, void*, long);
void	*emalloc(ulong);
char*	geterrstr(void);
Drive*	mmcprobe(Scsi*);

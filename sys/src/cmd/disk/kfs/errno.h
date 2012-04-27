enum{
	Enevermind,	/* never mind */
	Enofd,		/* no free file descriptors */
	Efidinuse,		/* fid already in use */
	Ebadfd,		/* fid out of range or not open */
	Ebadusefd,	/* inappropriate use of fid */
	Ebadarg,	/* bad arg in system call */
	Enonexist,	/* file does not exist */
	Efilename,	/* file name syntax */
	Ebadchar,	/* bad character in file name */
	Ebadsharp,	/* unknown device in # filename */
	Ebadexec,	/* a.out header invalid */
	Eioload,	/* i/o error in demand load */
	Eperm,		/* permission denied */
	Enotdir,	/* not a directory */
	Enochild,	/* no living children */
	Enoseg,		/* no free segments */
	Ebuf,		/* buffer wrong size */
	Ebadmount,	/* inconsistent mount */
	Enomount,	/* mount table full */
	Enomntdev,	/* no free mount devices */
	Eshutdown,	/* mounted device shut down */
	Einuse,		/* device or object already in use */
	Eio,		/* i/o error */
	Eisdir,		/* file is a directory */
	Ebaddirread,	/* directory read not quantized */
	Esegaddr,	/* illegal segment addresses or size */
	Enoenv,		/* no free environment resources */
	Eprocdied,	/* process exited */
	Enocreate,	/* mounted directory forbids creation */
	Enotunion,	/* attempt to union with non-mounted directory */
	Emount,		/* inconsistent mount */
	Enosrv,		/* no free server slots */
	Egreg,		/* it's all greg's fault */
};

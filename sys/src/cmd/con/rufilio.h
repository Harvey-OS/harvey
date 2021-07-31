/*
 * general file and stream ioctls
 */

/*
 * for FIOINSLD
 */
struct	insld {
	short	ld;
	short	level;
};

/*
 * for passing files across streams
 */
struct	passfd {
	int	fd;
	short	uid;
	short	gid;
	short	nice;
	char	logname[8];
};

/*
 * file ioctls
 */
#define	FIOCLEX		(('f'<<8)|1)
#define	FIONCLEX	(('f'<<8)|2)
#define	FIOPUSHLD	(('f'<<8)|3)
#define	FIOPOPLD	(('f'<<8)|4)
#define	FIOLOOKLD	(('f'<<8)|5)
#define FIOINSLD	(('f'<<8)|6)
#define	FIOSNDFD	(('f'<<8)|7)
#define	FIORCVFD	(('f'<<8)|8)
#define	FIOACCEPT	(('f'<<8)|9)
#define	FIOREJECT	(('f'<<8)|10)
#define	FIONREAD	(('f'<<8)|127)

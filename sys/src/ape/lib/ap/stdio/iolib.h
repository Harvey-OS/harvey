/*
 * pANS stdio -- definitions
 * The following names are defined in the pANS:
 *	FILE	 	fpos_t		_IOFBF		_IOLBF		_IONBF
 *	BUFSIZ		EOF		FOPEN_MAX	FILENAME_MAX	L_tmpnam
 *	SEEK_CUR	SEEK_END	SEEK_SET	TMP_MAX		stderr
 *	stdin		stdout		remove		rename		tmpfile
 *	tmpnam		fclose		fflush		fopen		freopen
 *	setbuf		setvbuf		fprintf		fscanf		printf
 *	scanf		sprintf		sscanf		vfprintf	vprintf
 *	vsprintf	fgetc		fgets		fputc		fputs
 *	getc		getchar		gets		putc		putchar
 *	puts		ungetc		fread		fwrite		fgetpos
 *	fseek		fsetpos		ftell		rewind		clearerr
 *	feof		ferror		perror	
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

/*
 * Flag bits
 */
#define	BALLOC	1	/* did stdio malloc fd->buf? */
#define	LINEBUF	2	/* is stream line buffered? */
#define	STRING	4	/* output to string, instead of file */
#define APPEND	8	/* append mode output */
/*
 * States
 */
#define	CLOSED	0	/* file not open */
#define	OPEN	1	/* file open, but no I/O buffer allocated yet */
#define	RDWR	2	/* open, buffer allocated, ok to read or write */
#define	RD	3	/* open, buffer allocated, ok to read but not write */
#define	WR	4	/* open, buffer allocated, ok to write but not read */
#define	ERR	5	/* open, but an uncleared error occurred */
#define	END	6	/* open, but at eof */
char *strerror(int errno);
int _IO_setvbuf(FILE *);
FILE *_IO_sopenr(const char*);
FILE *_IO_sopenw(void);
char *_IO_sclose(FILE *);

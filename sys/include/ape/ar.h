#ifndef __AR_H
#define __AR_H
#ifndef _RESEARCH_SOURCE
   This header file is not defined in ANSI or POSIX
#endif

#define	ARMAG	"!<arch>\n"
#define	SARMAG	8

#define	ARFMAG	"`\n"

struct	ar_hdr
{
	char	ar_name[16];
	char	ar_date[12];
	char	ar_uid[6];
	char	ar_gid[6];
	char	ar_mode[8];
	char	ar_size[10];
	char	ar_fmag[2];
};
#define	SAR_HDR	60

#endif

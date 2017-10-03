/*
 * Copyright (c) 2002 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by Marshall
 * Kirk McKusick and Network Associates Laboratories, the Security
 * Research Division of Network Associates, Inc. under DARPA/SPAWAR
 * contract N66001-01-C-8035 ("CBOSS"), as part of the DARPA CHATS
 * research program.
 *
 * Copyright (c) 1983, 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <u.h>
#include <libc.h>

#include <ctype.h>
#include <ufs/ufsdat.h>
#include <ffs/fs.h>
#include <ufs/libufs.h>

#include "newfs.h"

int	erasecontents;		/* Erase previous disk contents */
int	addvolumelabel;		/* add a volume label */
int	createfs = 1;		/* run without writing file system */
int	regressiontest;		/* regression test */
int	enablesu;		/* enable soft updates for file system */
int	enablesuj;		/* enable soft updates journaling for filesys */
int	exitflag = 0;		/* exit in middle of newfs for testing */
int	enablemultilabel;	/* enable multilabel for file system */
int	createsnapdir = 1;	/* do not create .snap directory */
int	enabletrim;		/* enable TRIM */
int64_t fssize;			/* file system size */
//off_t	mediasize;		/* device size */
int	sectorsize;		/* bytes/sector */
//int	realsectorsize;		/* bytes/sector in hardware */
int	fsize = 0;		/* fragment size */
int	bsize = 0;		/* block size */
int	maxbsize = 0;		/* maximum clustering */
int	maxblkspercg = MAXBLKSPERCG; /* maximum blocks per cylinder group */
int	minfree = MINFREE;	/* free space threshold */
int	metaspace;		/* space held for metadata blocks */
int	opt = DEFAULTOPT;	/* optimization preference (space or time) */
int	density;		/* number of bytes per inode */
int	maxcontig = 0;		/* max contiguous blocks to allocate */
int	maxbpg;			/* maximum blocks per file in a cyl group */
int	avgfilesize = AVFILESIZ;/* expected average file size */
int	avgfilesperdir = AFPDIR;/* expected number of files per directory */
char	*volumelabel = nil;	/* volume label for filesystem */
Uufsd	disk;		/* libufs disk structure */

//static char	device[MAXPATHLEN];
//static u_char   bootarea[BBSIZE];
//static int	is_file;		/* work on a file, not a device */
//static char	*dkname;

//static void getfssize(intmax_t *, const char *p, intmax_t, intmax_t);
//static struct disklabel *getdisklabel(char *s);
static void usage();

static int
parseint(const char *str, int *val)
{
	if (str == nil) {
		return 0;
	}
	*val = atoi(str);
	return 1;
}

static int
parsei64(const char *str, int64_t *val)
{
	if (str == nil) {
		return 0;
	}
	*val = atoll(str);
	return 1;
}

void
main(int argc, char *argv[])
{
	ARGBEGIN {
	default:
		usage();
		exits(nil);

	case 'E':
		erasecontents = 1;
		break;
	case 'L':
	{
		volumelabel = ARGF();
		if (volumelabel == nil) {
			fprint(2, "No volume label specified\n");
			exits("no volume label");
		}

		int i = -1;
		while (isalnum(volumelabel[++i]))
			;
		if (volumelabel[i] != '\0') {
			fprint(2, "Volume label characters must be alphanumeric.\n");
			exits("bad volume label characters");
		}
		if (strlen(volumelabel) >= MAXVOLLEN) {
			fprint(2, "Volume label is too long. Length is longer than %d.\n",
			    MAXVOLLEN);
			exits("bad volume label length");
		}
		addvolumelabel = 1;
		break;
	}
	case 'N':
		createfs = 0;
		break;
	case 'R':
		regressiontest = 1;
		break;
	case 'S':
	{
		if (!parseint(ARGF(), &sectorsize) || sectorsize <= 0) {
			fprint(2, "Invalid sector size\n");
			exits("bad file sector size");
		}
		break;
	}
	case 'U':
		enablesu = 1;
		break;
	case 'X':
		exitflag++;
		break;
	case 'a':
	{
		if (!parseint(ARGF(), &maxcontig) || maxcontig <= 0) {
			fprint(2, "Invalid blocks per cylinder group\n");
			exits("bad blocks per cylinder group");
		}
		break;
	}
	case 'b':
	{
		if (!parseint(ARGF(), &bsize) || bsize < MINBSIZE) {
			fprint(2, "Block size too small, min is: %d\n", MINBSIZE);
			exits("block size too small");
		} else if (bsize > MAXBSIZE) {
			fprint(2, "Block size too large, max is: %d\n", MAXBSIZE);
			exits("block size too large");
		}
		break;
	}
	case 'c':
	{
		if (!parseint(ARGF(), &maxblkspercg) || maxblkspercg <= 0) {
			fprint(2, "Invalid sector size\n");
			exits("bad file sector size");
		}
		break;
	}
	case 'd':
	{
		if (!parseint(ARGF(), &maxbsize) || maxbsize < MINBSIZE) {
			fprint(2, "Invalid extent block size\n");
			exits("bad file sector size");
		}
		break;
	}
	case 'e':
	{
		if (!parseint(ARGF(), &maxbpg) || maxbpg <= 0) {
			fprint(2, "Invalid blocks per file in a cylinder group\n");
			exits("bad blocks per file in a cylinder group");
		}
		break;
	}
	case 'f':
	{
		if (!parseint(ARGF(), &fsize) || fsize <= 0) {
			fprint(2, "Invalid fragment size\n");
			exits("bad fragment size");
		}
		break;
	}
	case 'g':
	{
		if (!parseint(ARGF(), &avgfilesize) || avgfilesize <= 0) {
			fprint(2, "Invalid average file size\n");
			exits("bad average file size");
		}
		break;
	}
	case 'h':
	{
		if (!parseint(ARGF(), &avgfilesperdir) || avgfilesperdir <= 0) {
			fprint(2, "Invalid average files per dir\n");
			exits("bad average files per dir");
		}
		break;
	}
	case 'i':
	{
		if (!parseint(ARGF(), &density) || density <= 0) {
			fprint(2, "Invalid bytes per inode\n");
			exits("bad bytes per inode");
		}
		break;
	}
	case 'j':
		enablesuj = 1;
		enablesu = 1;
		break;
	case 'k':
	{
		if (!parseint(ARGF(), &metaspace)) {
			fprint(2, "Invalid metadata space\n");
			exits("bad metadata space");
		}
		if (metaspace == 0) {
			/* force to stay zero in mkfs */
			metaspace = -1;
		}
		break;
	}
	case 'l':
		enablemultilabel = 1;
		break;
	case 'm':
	{
		if (!parseint(ARGF(), &minfree) || minfree > 99) {
			fprint(2, "Invalid free space\n");
			exits("bad free space");
		}
		break;
	}
	case 'n':
		createsnapdir = 0;
		break;
	case 'o':
	{
		char *p = ARGF();
		if (strcmp(p, "space") == 0) {
			opt = FS_OPTSPACE;
		} else if (strcmp(p, "time") == 0) {
			opt = FS_OPTTIME;
		} else {
			fprint(2, "Invalid optimization preference: use `space' or `time'\n");
			exits("bad optimization preference");
		}
		break;
	}
	case 's':
	{
		if (!parsei64(ARGF(), &fssize) || fssize <= 0) {
			fprint(2, "Invalid file system size\n");
			exits("bad file system size");
		}
		break;
	}
	case 't':
		enabletrim = 1;
		break;
	} ARGEND

	if (argv[0] == nil) {
		usage();
		exits("usage");
	}

	char *filename = argv[0];

	memset(&disk, 0, sizeof(disk));
	disk.d_bsize = 1;
	disk.d_name = filename;
	if (createfs && ufs_disk_create(&disk) == -1) {
		fprint(2, "Can't create file %s\n", filename);
		exits("can't create file");
	}

	mkfs(filename);
	ufs_disk_close(&disk);

	if (enablesuj) {
		fprint(2, "Soft updates journalling not implemented yet\n");
		//if (execlp("tunefs", "newfs", "-j", "enable", special, nil) < 0)
		//	err(1, "Cannot enable soft updates journaling, tunefs");
		/* NOT REACHED */
	}
	exits(nil);
#if 0
	struct partition *pp;
	struct disklabel *lp;
	struct stat st;
	char *cp, *special;
	intmax_t reserved;
	int ch, i, rval;
	char part_name;		/* partition name, default to full disk */

	part_name = 'c';
	reserved = 0;
	while ((ch = getopt(argc, argv,
	    "EJL:NO:RS:T:UXa:b:c:d:e:f:g:h:i:jk:lm:no:p:r:s:t")) != -1)
		switch (ch) {
		case 'e':
			rval = expand_number_int(optarg, &maxbpg);
			if (rval < 0 || maxbpg <= 0)
			  errx(1, "%s: bad blocks per file in a cylinder group",
				    optarg);
			break;
		case 'f':
			rval = expand_number_int(optarg, &fsize);
			if (rval < 0 || fsize <= 0)
				errx(1, "%s: bad fragment size", optarg);
			break;
		case 'g':
			rval = expand_number_int(optarg, &avgfilesize);
			if (rval < 0 || avgfilesize <= 0)
				errx(1, "%s: bad average file size", optarg);
			break;
		case 'h':
			rval = expand_number_int(optarg, &avgfilesperdir);
			if (rval < 0 || avgfilesperdir <= 0)
			       errx(1, "%s: bad average files per dir", optarg);
			break;
		case 'i':
			rval = expand_number_int(optarg, &density);
			if (rval < 0 || density <= 0)
				errx(1, "%s: bad bytes per inode", optarg);
			break;
		case 'k':
			if ((metaspace = atoi(optarg)) < 0)
				errx(1, "%s: bad metadata space %%", optarg);
			if (metaspace == 0)
				/* force to stay zero in mkfs */
				metaspace = -1;
			break;
		case 'm':
			if ((minfree = atoi(optarg)) < 0 || minfree > 99)
				errx(1, "%s: bad free space %%", optarg);
			break;
		case 'o':
			if (strcmp(optarg, "space") == 0)
				opt = FS_OPTSPACE;
			else if (strcmp(optarg, "time") == 0)
				opt = FS_OPTTIME;
			else
				errx(1, 
		"%s: unknown optimization preference: use `space' or `time'",
				    optarg);
			break;
		case 'r':
			errno = 0;
			reserved = strtoimax(optarg, &cp, 0);
			if (errno != 0 || cp == optarg ||
			    *cp != '\0' || reserved < 0)
				errx(1, "%s: bad reserved size", optarg);
			break;

		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	special = argv[0];
	if (!special[0])
		err(1, "empty file/special name");
	cp = strrchr(special, '/');
	if (cp == nil) {
		/*
		 * No path prefix; try prefixing _PATH_DEV.
		 */
		snprintf(device, sizeof(device), "%s%s", _PATH_DEV, special);
		special = device;
	}

	if (is_file) {
		/* bypass ufs_disk_fillout_blank */
		bzero( &disk, sizeof(disk));
		disk.d_bsize = 1;
		disk.d_name = special;
		disk.d_fd = open(special, O_RDONLY);
		if (disk.d_fd < 0 ||
		    (createfs && ufs_disk_write(&disk) == -1))
			errx(1, "%s: ", special);
	} else if (ufs_disk_fillout_blank(&disk, special) == -1 ||
	    (createfs && ufs_disk_write(&disk) == -1)) {
		if (disk.d_error != nil)
			errx(1, "%s: %s", special, disk.d_error);
		else
			err(1, "%s", special);
	}
	if (fstat(disk.d_fd, &st) < 0)
		err(1, "%s", special);
	if ((st.st_mode & S_IFMT) != S_IFCHR) {
		warn("%s: not a character-special device", special);
		is_file = 1;	/* assume it is a file */
		dkname = special;
		if (sectorsize == 0)
			sectorsize = 512;
		mediasize = st.st_size;
		/* set fssize from the partition */
	} else {
	    if (sectorsize == 0)
		if (ioctl(disk.d_fd, DIOCGSECTORSIZE, &sectorsize) == -1)
		    sectorsize = 0;	/* back out on error for safety */
	    if (sectorsize && ioctl(disk.d_fd, DIOCGMEDIASIZE, &mediasize) != -1)
		getfssize(&fssize, special, mediasize / sectorsize, reserved);
	}
	pp = nil;
	lp = getdisklabel(special);
	if (lp != nil) {
		if (!is_file) /* already set for files */
			part_name = special[strlen(special) - 1];
		if ((part_name < 'a' || part_name - 'a' >= MAXPARTITIONS) &&
				!isdigit(part_name))
			errx(1, "%s: can't figure out file system partition",
					special);
		cp = &part_name;
		if (isdigit(*cp))
			pp = &lp->d_partitions[RAW_PART];
		else
			pp = &lp->d_partitions[*cp - 'a'];
		if (pp->p_size == 0)
			errx(1, "%s: `%c' partition is unavailable",
			    special, *cp);
		if (pp->p_fstype == FS_BOOT)
			errx(1, "%s: `%c' partition overlaps boot program",
			    special, *cp);
		getfssize(&fssize, special, pp->p_size, reserved);
		if (sectorsize == 0)
			sectorsize = lp->d_secsize;
		if (fsize == 0)
			fsize = pp->p_fsize;
		if (bsize == 0)
			bsize = pp->p_frag * pp->p_fsize;
		if (is_file)
			part_ofs = pp->p_offset;
	}
	if (sectorsize <= 0)
		errx(1, "%s: no default sector size", special);
	if (fsize <= 0)
		fsize = MAX(DFL_FRAGSIZE, sectorsize);
	if (bsize <= 0)
		bsize = MIN(DFL_BLKSIZE, 8 * fsize);
	if (minfree < MINFREE && opt != FS_OPTSPACE) {
		fprint(2, "Warning: changing optimization to space ");
		fprint(2, "because minfree is less than %d%%\n", MINFREE);
		opt = FS_OPTSPACE;
	}
	realsectorsize = sectorsize;
	if (sectorsize != DEV_BSIZE) {		/* XXX */
		int secperblk = sectorsize / DEV_BSIZE;

		sectorsize = DEV_BSIZE;
		fssize *= secperblk;
		if (pp != nil)
			pp->p_size *= secperblk;
	}
#endif // 0
}

#if 0
void
getfssize(intmax_t *fsz, const char *s, intmax_t disksize, intmax_t reserved)
{
	intmax_t available;

	available = disksize - reserved;
	if (available <= 0)
		errx(1, "%s: reserved not less than device size %jd",
		    s, disksize);
	if (*fsz == 0)
		*fsz = available;
	else if (*fsz > available)
		errx(1, "%s: maximum file system size is %jd",
		    s, available);
}

struct disklabel *
getdisklabel(char *s)
{
	static struct disklabel lab;
	struct disklabel *lp;

	if (is_file) {
		if (read(disk.d_fd, bootarea, BBSIZE) != BBSIZE)
			err(4, "cannot read bootarea");
		if (bsd_disklabel_le_dec(
		    bootarea + (0 /* labeloffset */ +
				1 /* labelsoffset */ * sectorsize),
		    &lab, MAXPARTITIONS))
			errx(1, "no valid label found");

		lp = &lab;
		return &lab;
	}

	if (disktype) {
		lp = getdiskbyname(disktype);
		if (lp != nil)
			return (lp);
	}
	return (nil);
}

#endif // 0

static void
usage()
{
	fprint(2, "usage: newfs [ -fsoptions ] filename\n");
	fprint(2, "where fsoptions are:\n");
	fprint(2, "\t-E Erase previous disk content\n");
	fprint(2, "\t-L volume label to add to superblock\n");
	fprint(2, "\t-N do not create file system, just print out parameters\n");
	fprint(2, "\t-R regression test, suppress random factors\n");
	fprint(2, "\t-S sector size\n");
	fprint(2, "\t-U enable soft updates\n");
	fprint(2, "\t-a maximum contiguous blocks\n");
	fprint(2, "\t-b block size\n");
	fprint(2, "\t-c blocks per cylinders group\n");
	fprint(2, "\t-d maximum extent size\n");
	fprint(2, "\t-e maximum blocks per file in a cylinder group\n");
	fprint(2, "\t-f frag size\n");
	fprint(2, "\t-g average file size\n");
	fprint(2, "\t-h average files per directory\n");
	fprint(2, "\t-i number of bytes per inode\n");
	fprint(2, "\t-j enable soft updates journaling\n");
	fprint(2, "\t-k space to hold for metadata blocks\n");
	fprint(2, "\t-l enable multilabel MAC\n");
	fprint(2, "\t-n do not create .snap directory\n");
	fprint(2, "\t-m minimum free space %%\n");
	fprint(2, "\t-o optimization preference (`space' or `time')\n");
	fprint(2, "\t-s file system size (sectors)\n");
	fprint(2, "\t-t enable TRIM\n");
	exits("usage");
}
